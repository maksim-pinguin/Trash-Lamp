var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
    console.log('Server: ', e.data);
    setBrightnessValue(parseInt(e.data.match(/(\d+)$/)[0], 10));
};
connection.onclose = function () {
    console.log('WebSocket connection closed');
};

function setBrightnessValue(value) {
    document.getElementById('brightnessValue').value = value;
    console.log('BrightnessValue got set to: ' + value);
}

function sendBrightnessValue() {
    var brightnessValue = document.getElementById('brightnessValue').value;
    console.log('Sending BrightnessValue: ' + brightnessValue);
    connection.send('b' + brightnessValue);
}

function sendLoopingIntervalValue() {
    var loopingIntervalValue = document.getElementById('loopingIntervalValue').value;
    console.log('Sending LoopingIntervalValue: ' + loopingIntervalValue);
    connection.send('i' + loopingIntervalValue);
}
function updateStyles(){
    if (loopingEnableValue) {
        document.getElementById('brightnessValue').disabled = true;
        document.getElementById('brightnessValue').className = 'disabled';
        document.getElementById('loopingIntervalValue').style.display = 'block';
        document.getElementById('loopingIntervalValue').className = 'enabled';
        document.getElementById('loopingIntervalValue').disabled = false;
        document.getElementById('loopingIntervalValueLable').style.display = 'block';
        document.getElementById('loopingIntervalValueLable').disabled = false;
        document.getElementById('loopingIntervalValueLable').className = 'enabled';
    } else {
        document.getElementById('brightnessValue').disabled = false;
        document.getElementById('brightnessValue').className = 'enabled';
        document.getElementById('loopingIntervalValue').style.display = 'none';
        document.getElementById('loopingIntervalValue').className = 'disabled';
        document.getElementById('loopingIntervalValue').disabled = true;
        document.getElementById('loopingIntervalValueLable').style.display = 'none';
        document.getElementById('loopingIntervalValueLable').className = 'disabled';
        document.getElementById('loopingIntervalValueLable').disabled = true;
    }

}

function LCDPowerToogleOnClick(e) {
    setLCDPowerState(e.target.checked);
}

function setLCDPowerState(powerState) {
    LCDPower = powerState;
    if (toggleLCDPowerValue) {
        console.log("set LCD power state to on");
    } else {
        console.log("set LCD power state to off");
        connection.send("p");
    }
    updateStyles();
}

document.getElementById("toggleLCDPowerValue").checked = false;
document.getElementById("enableLoopingValue").checked = false;

function enableLooping() {
    loopingEnableValue = document.getElementById("enableLoopingValue").checked;
    console.log("Turned " + (loopingEnableValue?"on ":"off ") + "looping effect!");
    updateStyles();
}
