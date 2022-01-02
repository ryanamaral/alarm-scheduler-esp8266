#include <pgmspace.h>
char main_js[] PROGMEM = R"=====(
var connection;
function prefillClockInputs() {
    var dateOn = new Date();
    var dateOff = new Date();
    dateOn.setMinutes(dateOn.getMinutes() + 1);
    dateOff.setMinutes(dateOff.getMinutes() + 2);
    var inputTimeOn = document.getElementById('inputTimeOn');
    var inputTimeOff = document.getElementById('inputTimeOff');
    inputTimeOn.value = dateOn.toLocaleTimeString(navigator.language, {hour: '2-digit', minute:'2-digit'});
    inputTimeOff.value = dateOff.toLocaleTimeString(navigator.language, {hour: '2-digit', minute:'2-digit'});
}
function syncSystemTime() {
    var seconds = Math.round(Date.now() / 1000);
    var uri = '/systemTime?timestamp=' + seconds;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', uri);
    xhr.send();
}
function updateLightState(mode) {
    var uri = '/updateLightState?mode=' + mode;
    uri += '&duration=0';
    var xhr = new XMLHttpRequest();
    xhr.open('GET', uri);
    xhr.send();
}
function switchOn() {
    updateLightState(1);
}
function switchOff() {
    updateLightState(0);
}
function animationOn() {
    updateLightState(3);
}
function animationOff() {
    updateLightState(2);
}
function scheduleAlarmOn() {
    var uri = '/scheduleAlarm'
    uri += '&power=1';
    uri += '&duration=' + document.getElementById('durationSchedule').value;
    var inputTimeOn = document.getElementById('inputTimeOn');
    uri += '&hh=' + inputTimeOn.valueAsDate.getUTCHours();
    uri += '&mm=' + inputTimeOn.valueAsDate.getMinutes();
    var xhr = new XMLHttpRequest();
    xhr.open('GET', uri);
    xhr.send();
}
function scheduleAlarmOff() {
    var uri = '/scheduleAlarm'
    uri += '&power=0';
    uri += '&duration=' + document.getElementById('durationSchedule').value;
    var inputTimeOff = document.getElementById('inputTimeOff');
    uri += '&hh=' + inputTimeOff.valueAsDate.getUTCHours();
    uri += '&mm=' + inputTimeOff.valueAsDate.getMinutes();
    var xhr = new XMLHttpRequest();
    xhr.open('GET', uri);
    xhr.send();
}
function cancelScheduledAlarms() {
    var uri = '/cancelScheduledAlarms';
    var xhr = new XMLHttpRequest();
    xhr.open('GET', uri);
    xhr.send();
}
function updateBrightnessView(brightness) {
    if (brightness === '') return;
    var progress = document.getElementById('brightnessProgress');
    progress.innerHTML = brightness;
    var brightnessSlider = document.getElementById('brightnessSlider');
    brightnessSlider.value = brightness;
}
function updateTimestamp(timestamp) {
    if (timestamp === '') return;
    var timestampView = document.getElementById('timestamp');
    timestampView.innerHTML = timestamp;        
}
function updateTimestampBrowser(timestamp) {
    if (timestamp === '') return;
    var timestampView = document.getElementById('timestampBrowser');
    timestampView.innerHTML = timestamp;        
}
function updateConnectionStatus(status) {
    var statusBadge = document.getElementById('statusBadge');
    if (status == undefined) {
        statusBadge.className = 'btn btn-danger';
        statusBadge.innerText = 'Disconnected';
    } else {
        statusBadge.className = 'btn btn-success';
        statusBadge.innerText = 'Connected';
    }
}
function updateLightStatus(power) {
    var statusLight = document.getElementById('statusLight');
    switch (power) {
      case 0:
        console.log('LIGHTS_OFF');
        var offHtml = '<span class=\'badge badge-danger\'>OFF</span>';
        statusLight.innerHTML = 'Current state: ' + offHtml;
        break;
      case 1:
        console.log('LIGHTS_ON');
        var onHtml = '<span class=\'badge badge-success\'>ON</span>';
        statusLight.innerHTML = 'Current state: ' + onHtml;
        break;
      case 2:
        console.log('LIGHTS_TURNING_OFF');
        var badge = '<span class=\'badge badge-warning\'>Switching OFF...</span>';
        statusLight.innerHTML = 'Current state: ' + badge;
        break;
      case 3:
        console.log('LIGHTS_TURNING_ON');
        var badge = '<span class=\'badge badge-warning\'>Switching ON...</span>';
        statusLight.innerHTML = 'Current state: ' + badge;
        break;
      default:
        console.log('Invalid power mode: ${power}');
    }
}
function updateBuildView(status) {
    var navbarTitle = document.getElementById('navbarTitle');
    var title = 'ESP8266 Alarm Scheduler ';
    title += status.version;    
    navbarTitle.innerHTML = title;
}
function displayStatus(status) {
    updateConnectionStatus(status);
    if (status == undefined) return;

    updateBuildView(status);

    updateBrightnessView(status.brightness);

    updateTimestamp(status.timestamp);
    
    updateLightStatus(status.power);
}
function refreshStatus() {
    var statusRequest = new XMLHttpRequest();
    statusRequest.open('GET', '/status');
    statusRequest.onreadystatechange = function() {
        if (statusRequest.readyState == 4) {
            if (statusRequest.status == 200) {
                displayStatus(JSON.parse(statusRequest.response));
            } else {
                displayStatus();
            }
        }
    }
    statusRequest.send();
}
function refreshTimestampBrowser() {
    var date = new Date();
    var dateString = date.toLocaleTimeString(navigator.language, {hour: '2-digit', minute:'2-digit', second:'2-digit'});
    updateTimestampBrowser(dateString);
}
setInterval(refreshTimestampBrowser, 1000);
document.addEventListener('DOMContentLoaded', function() {
    syncSystemTime();
    refreshTimestampBrowser();
    prefillClockInputs();
    refreshStatus();
    
    connection = new WebSocket('ws://10.0.0.1:81/', ['arduino']);
    connection.onopen = function () {
      connection.send('Connect ' + new Date());
    };
    connection.onerror = function (error) {
      console.log('WebSocket Error ', error);
    };
    connection.onmessage = function (m) {
      if (m != undefined) {
        console.log('OnMessage: ', m.data);
        var json = JSON.parse(m.data);
        if (json != undefined) {
            if (json.name == 'brightness') {
                updateBrightnessView(json.value);
                
            } else if (json.name == 'power') {
                updateLightStatus(json.value);
                
            } else if (json.name == 'timestamp') {
                updateTimestamp(json.value);
            }
        } else {
            console.log('error parsing json response');
        } 
      }
    };
}, false);
)=====";
