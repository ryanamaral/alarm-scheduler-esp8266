// Alarm Scheduler ESP-8266
//
// ESP8266 Schedule alarms to occur at specific times via WebUI
//
// Hardware:
// * ESP-01S (ESP-8266)
//
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <TimeLib.h> // https://github.com/PaulStoffregen/Time (v1.6.1)
#include "TimeAlarmsESP8266.h"
// Base code: https://github.com/PaulStoffregen/TimeAlarms (v1.6.0)
// Plus this changes: https://github.com/gmag11/TimeAlarms/commit/c8363f6ee96ad18e27f2b05460549c0157ab8956
// Because of Alarm Problem on ESP8266: https://github.com/PaulStoffregen/TimeAlarms/issues/23
#include <math.h> // https://www.arduino.cc/en/Reference/MathHeader


// (bash) $ date "+%d.%m.%Y %H:%M:%S %s"
const unsigned long BUILD_TIMESTAMP = 1641121439; // 02.01.2022 11:03:59 
const String BUILD_VERSION_NAME = "0.2";

const char *apHostname = "scheduler"; // http://scheduler.local
const char *ssid = "AlarmScheduler-2022";
const char *psk = "ThmRS9L8aN7gm8pdY9STH8hMdazCvV5TwK6LNqz2WpEW6cUpG7bwZcuehHfuNGa";
// https://zxing.org/w/chart?cht=qr&chs=350x350&chld=L&choe=UTF-8&chl=WIFI%3AS%3AAlarmScheduler-2022%3BT%3AWPA%3BP%3AThmRS9L8aN7gm8pdY9STH8hMdazCvV5TwK6LNqz2WpEW6cUpG7bwZcuehHfuNGa%3B%3B
// Tool to generate WiFi QR-Codes: http://zxing.appspot.com/generator/

extern const char index_html[];
extern const char style_css[];
extern const char main_js[];

#define GPIO_LED 2  // GPIO2 ESP-01
#define PORT_HTTP 80
#define PORT_WEBSOCKET 81

ESP8266WebServer server(PORT_HTTP);
WebSocketsServer webSocket = WebSocketsServer(PORT_WEBSOCKET);

/*
   WiFi AP + DNS
*/
#define MASK "255.255.255.128"
#define PORT_DNS 53
IPAddress staticIpAddress(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 128); // = 126 usable IP addresses
const int ssidMaxClient = 2;
const int ssidChannel = 7;
const bool ssidHidden = false;
DNSServer dnsServer;

/*
   States
*/
enum State {
  LIGHTS_OFF = 0,
  LIGHTS_ON = 1,
  LIGHTS_TURNING_OFF = 2,
  LIGHTS_TURNING_ON = 3
};
enum State lightState = LIGHTS_OFF;

static AlarmId turnLightOnAlarm;
static AlarmId turnLightOffAlarm;
static uint8_t connectedClients = 0;

#define MAX_LONG 1410065407 // 1410065407 is the MAX
unsigned long lastLedStatusUpdateMillis = MAX_LONG;
unsigned long lastTimestampsMillis = MAX_LONG;

int brightness = 0;
int fadeAmount = 3;
const int DEFAULT_FRAME_RATE = 9; // in ms
int frameRate = DEFAULT_FRAME_RATE; // in ms


//////////
// LOOP //
//////////

void loop() {
  unsigned long now = millis();

  // DNS
  dnsServer.processNextRequest();

  // WebSockets
  webSocket.loop();

  // Listen for HTTP requests
  server.handleClient();

  switch (lightState) {
    case LIGHTS_OFF: {
        brightness = 0;
        digitalWrite(GPIO_LED, HIGH); // inverted logic
        break;
      }
    case LIGHTS_ON: {
        brightness = 255;
        digitalWrite(GPIO_LED, LOW); // inverted logic
        break;
      }
    default: break; // do nothing
  }

  if (now - lastLedStatusUpdateMillis > frameRate) {
    switch (lightState) {

      case LIGHTS_TURNING_ON: {
          int invertedBrightness = 255 - brightness;
          analogWrite(GPIO_LED, invertedBrightness);

          brightness = brightness + fadeAmount;
          if (brightness >= 255) {
            brightness = 255;
            lightState = LIGHTS_ON;
            broadcastInt("power", lightState);
          }
          broadcastInt("brightness", brightness);
          break;
        }

      case LIGHTS_TURNING_OFF: {
          int invertedBrightness = 255 - brightness;
          analogWrite(GPIO_LED, invertedBrightness);

          brightness = brightness - fadeAmount;
          if (brightness <= 0) {
            brightness = 0;
            lightState = LIGHTS_OFF;
            broadcastInt("power", lightState);
          }
          broadcastInt("brightness", brightness);
          break;
        }

      default: break; // do nothing
    }
    lastLedStatusUpdateMillis = now;
  }

  if ((now - lastTimestampsMillis > 1000) && connectedClients > 0) {
    // send esp time every second to connected clients
    broadcastString("timestamp", getCurrentTime());
    lastTimestampsMillis = now;
  }

  Alarm.delay(0);
}

/**
  Param 'duration' is in seconds
*/
void updateFrameRate(int duration) {
  if (duration == 0) {
    frameRate = DEFAULT_FRAME_RATE;
  } else {
    frameRate = round((duration * 1000) / (255 / fadeAmount));
  }
}


/////////////////////////
// Webserver Functions //
/////////////////////////

void setupWebServer() {
  Serial.println("HTTP server setup");
  server.on("/", handleIndexHtml);
  server.on("/style.css", handleStyleCss);
  server.on("/main.js", handleMainJs);
  server.on("/status", handleStatus);
  server.on("/updateLightState", handleLightStateUpdate);
  server.on("/systemTime", handleSystemTime);
  server.on("/scheduleAlarm", handleScheduleAlarm);
  server.on("/cancelScheduledAlarms", cancelScheduledAlarms);
  server.on("/generate_204", handleCaptivePortal);              // Android captive portal
  server.on("/gen_204", handleCaptivePortal);
  server.on("/fwlink", handleCaptivePortal);                    // Microsoft captive portal
  server.on("/connecttest.txt", handleCaptivePortal);           // Microsoft Windows 10
  server.on("/hotspot-detect.html", handleCaptivePortal);       // Apple devices
  server.on("/library/test/success.html", handleCaptivePortal);
  server.on("/kindle-wifi/wifistub.html", handleCaptivePortal); // Kindle
  server.onNotFound(handleNotFound);
  server.begin(); // WebServer start

  Serial.println("HTTP server started.");
  Serial.println();
}

void handleIndexHtml() {
  server.send_P(200, "text/html", index_html);
  // send_P(): send content directly from Flash PROGMEM to client
}

void handleStyleCss() {
  server.send_P(200, "text/css", style_css);
  // send_P(): send content directly from Flash PROGMEM to client
}

void handleMainJs() {
  server.send_P(200, "application/javascript", main_js);
  // send_P(): send content directly from Flash PROGMEM to client
}

void handleNotFound() {
  printRequest();

  // send 404 error message
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", "404 Page Not Found");
}

/*
   Callback for Captive Portal web page

   Android: http://10.0.0.1/generate_204
   Microsoft: http://10.0.0.1/fwlink
*/
void handleCaptivePortal() {
  printRequest();

  Serial.println("Request redirected to captive portal");

  server.sendHeader("Location", String("http://") + toStringIp(staticIpAddress), true);
  server.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  //server.client().stop(); // Stop is needed because we sent no content length
}

/**
   Callback for status endpoint
*/
void handleStatus() {
  char buf[256] = {0};
  sprintf(
    buf,
    "{\"power\":%d, \"brightness\":%d, \"timestamp\":%d, \"version\":\"%s\"}",
    lightState,
    brightness,
    BUILD_TIMESTAMP,
    BUILD_VERSION_NAME
  );
  server.send(200, "application/json", buf);
}

void handleLightStateUpdate() {
  printRequest();

  if (!server.hasArg("mode")) {
    server.send(400);
    return;
  }
  if (server.hasArg("duration")) {
    int duration = server.arg("duration").toInt();
    updateFrameRate(duration);
  }
  int mode = server.arg("mode").toInt();
  switch (mode) {

    case LIGHTS_ON:
      handleTurnLightOn();
      server.send(200);
      break;

    case LIGHTS_OFF:
      handleTurnLightOff();
      server.send(200);
      break;

    case LIGHTS_TURNING_ON:
      handleTurnLightOnAnimated();
      server.send(200);
      break;

    case LIGHTS_TURNING_OFF:
      handleTurnLightOffAnimated();
      server.send(200);
      break;

    default:
      server.send(404);
  }
}

void handleTurnLightOn() {
  if (lightState != LIGHTS_ON) {
    Serial.println("Switch ON...");
    frameRate = DEFAULT_FRAME_RATE;
    lightState = LIGHTS_ON;
    brightness = 255;
    broadcastInt("power", lightState);
    broadcastInt("brightness", brightness);
  }
}

void handleTurnLightOff() {
  if (lightState != LIGHTS_OFF) {
    Serial.println("Switch OFF...");
    frameRate = DEFAULT_FRAME_RATE;
    lightState = LIGHTS_OFF;
    brightness = 0;
    broadcastInt("power", lightState);
    broadcastInt("brightness", brightness);
  }
}

void handleTurnLightOnAnimated() {
  if (lightState != LIGHTS_ON && lightState != LIGHTS_TURNING_ON) {
    Serial.println("Switch ON Animated...");
    brightness = 0;
    lightState = LIGHTS_TURNING_ON;
    broadcastInt("brightness", brightness);
    broadcastInt("power", lightState);
  }
}

void handleTurnLightOffAnimated() {
  if (lightState != LIGHTS_OFF && lightState != LIGHTS_TURNING_OFF) {
    Serial.println("Switch OFF Animated...");
    brightness = 255;
    lightState = LIGHTS_TURNING_OFF;
    broadcastInt("brightness", brightness);
    broadcastInt("power", lightState);
  }
}

void turnLightOnScheduled() {
  Serial.println("Turn Light On Scheduled");
  handleTurnLightOnAnimated();
}

void turnLightOffScheduled() {
  Serial.println("Turn Light Off Scheduled");
  handleTurnLightOffAnimated();
}

void handleScheduleAlarm() {
  printRequest();

  if (!server.hasArg("power") || !server.hasArg("duration")
      || !server.hasArg("hh") || !server.hasArg("mm")) {
    server.send(400);
    return;
  }
  int power = server.arg("power").toInt();
  Serial.print("Power: ");
  Serial.println(power);
  int hh = server.arg("hh").toInt();
  int mm = server.arg("mm").toInt();
  Serial.print("HH: ");
  Serial.println(hh);
  Serial.print("MM: ");
  Serial.println(mm);
  int duration = server.arg("duration").toInt();
  updateFrameRate(duration);
  if (power == 0) {
    turnLightOffAlarm = Alarm.alarmRepeat(hh, mm, 0, turnLightOffScheduled);
    Serial.print("Alarm OFF Set");
  } else {
    turnLightOnAlarm = Alarm.alarmRepeat(hh, mm, 0, turnLightOnScheduled);
    Serial.print("Alarm ON Set");
  }
  server.send(200);
}

void cancelScheduledAlarms() {
  printRequest();

  Alarm.free(turnLightOnAlarm);
  Alarm.free(turnLightOffAlarm);
  turnLightOnAlarm = dtINVALID_ALARM_ID;
  turnLightOffAlarm = dtINVALID_ALARM_ID;
}

void handleSystemTime() {
  printRequest();

  if (!server.hasArg("timestamp")) {
    server.send(400);
    return;
  }
  Serial.println("ESP timestamp: ");
  Serial.println(now()); // seconds

  unsigned long currentTimestamp = server.arg("timestamp").toInt();
  Serial.println("New timestamp: ");
  Serial.println(currentTimestamp);

  if (currentTimestamp >= BUILD_TIMESTAMP) {
    if (timeStatus() != timeSet) {
      // Sync system clock to the time received
      setTime(currentTimestamp);
      Serial.println("ESP time set");
      Serial.println(currentTimestamp);
    } else {
      Serial.println("ESP time is already set");
      Serial.println(now()); // seconds
    }
    server.send(200);
  } else {
    server.send(400);
  }
}


//////////
// Time //
//////////

time_t requestTimeSync() {
  return 0; // return 0 if unable to return time; time will be sent later
}

String getCurrentTime() {
  char buf[6] = {0};
  sprintf(buf, "%s:%s:%s",
          getClockDigits(hour()),
          getClockDigits(minute()),
          getClockDigits(second())
         );
  return buf;
}

String getClockDigits(int digits) {
  char buf[2] = {0};
  if (digits < 10) {
    sprintf(buf, "0%d", digits);
  } else {
    sprintf(buf, "%d", digits);
  }
  return buf;
}


////////////
// Common //
////////////

/*
   Print the URI and query string of the current request
*/
void printRequest() {
  Serial.print("Received HTTP request for: ");
  Serial.print(server.uri());

  for (int i = 0; i < server.args(); i++) {
    if (i == 0) {
      Serial.print("?");
    }
    else {
      Serial.print("&");
    }
    Serial.print(server.argName(i));
    Serial.print("=");
    Serial.print(server.arg(i));
  }
  Serial.println();
}

/**
   IP to String
*/
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}


///////////////
// WebSocket //
///////////////

void broadcastBool(String name, bool value) {
  String json = "{\"name\":\"" + name + "\",\"value\":" + value + "}";
  webSocket.broadcastTXT(json);
}

void broadcastInt(String name, uint8_t value) {
  String json = "{\"name\":\"" + name + "\",\"value\":" + String(value) + "}";
  webSocket.broadcastTXT(json);
}

void broadcastString(String name, String value) {
  String json = "{\"name\":\"" + name + "\",\"value\":\"" + String(value) + "\"}";
  webSocket.broadcastTXT(json);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
        Serial.println("<WebSocketEvent> Disconnected");
        connectedClients--;
        break;
      }
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.print("<WebSocketEvent> Connected from: ");
        Serial.println(toStringIp(ip));
        connectedClients++;

        // send message to client
        webSocket.sendTXT(num, "Connected");
        break;
      }
    default: break;
  }
}


///////////////////////
// WiFi Access Point //
///////////////////////

void dnsSetup(IPAddress ipAddress) {
  Serial.print("Starting DNS server... ");

  // Setup the DNS server redirecting all http requests
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(PORT_DNS, "*", ipAddress);

  Serial.println("Initializing MDNS for local hostname on AP");
  if (MDNS.begin(apHostname)) {
    MDNS.addService("http", "tcp", PORT_HTTP);
    MDNS.addService("ws", "tcp", PORT_WEBSOCKET);
    
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(apHostname);
    Serial.println(".local");
  }
}

/*
   Setup WiFi AP + DNS (for redirect)
*/
void wifiSetupAccessPointMode() {
  Serial.print("Setting soft-AP configuration... ");
  WiFi.mode(WIFI_AP);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // default=MODEM_SLEEP
  Serial.println(WiFi.softAPConfig(staticIpAddress, staticIpAddress, subnet) ? "Ready" : "Failed!");

  Serial.print("Starting soft-AP... ");
  Serial.println(WiFi.softAP(ssid, psk, ssidChannel, ssidHidden, ssidMaxClient) ? "Ready" : "Failed!");

  delay(500); // Without delay I've seen the IP address blank
  IPAddress ipAddress = WiFi.softAPIP();

  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  Serial.print("AP IP address:\t");
  Serial.println(toStringIp(ipAddress));
  Serial.println();

  dnsSetup(ipAddress);
}


///////////
// SETUP //
///////////

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("------------------------------------");
  Serial.println("---------  Alarm Scheduler  --------");
  Serial.println("------------------------------------");
  Serial.println("----> MCU: Esp8266");
  Serial.println("----> Version: " + BUILD_VERSION_NAME);
  Serial.println("------------------------------------");
  Serial.println("------------- Changelog ------------");
  Serial.println("------------------------------------");
  Serial.println("----> State, brightness & timestamp");
  Serial.println("----> WebSockets to update UI");
  Serial.println("----> Lights control: On/Off");
  Serial.println("----> Animation duration UI");
  Serial.println("----> Sun Simulation Animation");
  Serial.println("----> Cancel scheduled alarms");
  Serial.println("----> Schedule sunrise & sunset");
  Serial.println("----> Sync Sytem Time");
  Serial.println("----> WiFi AP mode");
  Serial.println("------------------------------------");
  Serial.println("Starting...");
  delay(1000); // power-up safety delay

  // set up GPIO pins
  pinMode(GPIO_LED, OUTPUT);

  // In ES8266 the LED works with inverted logic
  // HIGH = Off, LOW = On
  // https://forum.arduino.cc/t/digitalwrite-high-low-inverted-solved/385210
  digitalWrite(GPIO_LED, HIGH); // off start

  Serial.println("Wifi setup");
  wifiSetupAccessPointMode();

  // setup web sockets
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  setupWebServer();

  // set function to call when sync required
  setSyncProvider(requestTimeSync);
}
