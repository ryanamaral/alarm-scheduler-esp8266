#include <pgmspace.h>
char index_html[] PROGMEM = R"=====(
<!doctype html>
<html lang='en' dir='ltr'>
<head>
    <meta http-equiv='Content-Type' content='text/html; charset=utf-8' />
    <meta name='viewport' content='width=device-width, initial-scale=1.0' />
    
    <title>ESP8266 Alarm Scheduler</title>

    <link type='text/css' href='style.css' rel='stylesheet'>
    
    <script type='text/javascript' src='main.js'></script>
</head>
<body>
    <div class='navbar navbar-expand-lg navbar-dark bg-primary mb-3'>
        <div class='container'>
            <span id='navbarTitle' class='navbar-brand'>ESP8266 Alarm Scheduler</span>
            <div class='navbar-right'>
                <span id='statusBadge' class='btn btn-danger' style='pointer-events: none;'>Disconnected</span>
            </div>
        </div>
    </div>
    <div id='container' class='container'>
        <div class='card mb-3'>
            <div class='card-body'>
                <span id='statusLight' style='font-size: 22px;'>Loading...</span>
                <br><br>
                <button class='btn btn-success mr-3' onclick='switchOn();'>Switch ON</button>
                <button class='btn btn-danger mr-3' onclick='switchOff();'>Switch OFF</button>
                <br><br>
                <input id='brightnessSlider' type='range' min='0' max='255' value='0' step='1' class='slider' disabled>
                <span>Brightness:</span>
                <b id='brightnessProgress'>0</b>
            </div>
        </div>
        <div class='card mb-3'>
            <div class='card-body'>
                <h4 class='card-title'>Test Animations</h4>
                <button class='btn btn-success mr-3' onclick='animationOn();'>Test 🌄Sunrise</button>
                <button class='btn btn-danger mr-3' onclick='animationOff();'>Test 🌅Sunset</button>
            </div>
        </div>
        <div class='card mb-3'>
            <div class='card-body'>
                <h4 class='card-title'>Schedule Animations</h4>
                <span style='padding-right: 12px;'>Browser time:</span>
                <span id='timestampBrowser' class='card-text'>Loading...</span>
                <br>
                <span style='padding-right: 6px;'>ESP8266 time:</span>
                <span id='timestamp' class='card-text'>Loading...</span>
                <br><br>
                <input type='time' id='inputTimeOn' value='14:00' class='form-control'>
                <label for='inputTimeOn' style='padding: 6px;'>🌄 <b>Sunrise time</b></label>
                <button class='btn btn-success mr-3' onclick='scheduleAlarmOn();'>Schedule</button>
                <br><br>
                <input type='time' id='inputTimeOff' value='08:00' class='form-control'>
                <label for='inputTimeOff' style='padding: 6px;'>🌅 <b>Sunset time</b></label>
                <button class='btn btn-success mr-3' onclick='scheduleAlarmOff();'>Schedule</button>
                <br><br>
                <input type='number' id='durationSchedule' name='duration' value='3' min='1' max='3600' label='Duration'>
                <b>Duration</b>
                <label for='durationSchedule'>(in seconds)</label>
                <br><br>
                <i class='card-title'>Note: All schedules are lost when the device is shutdown or rebooted.</i>
                <br><br>
                <button class='btn btn-danger mr-3' onclick='cancelScheduledAlarms();'>Delete Scheduled Events</button>
            </div>
        </div>
    </div>
</body>
</html>
)=====";
