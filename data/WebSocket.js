var LCDPower;
var loopingEnableValue;

document.getElementById("LCDPowerToggle").checked = false;
document.getElementById("loopingEffectToggle").checked = false;

var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
    //console.log('Server: ', e.data);
    if (e.data[0] === "b")
        setBrightnessValue(parseInt(e.data.match(/(\d+)$/)[0], 10));
    if (e.data[0] === "i")
        setIntervalValue(parseInt(e.data.match(/(\d+)$/)[0], 10));
    if (e.data[0] === "e")
        toggleLooping(true);
    if (e.data[0] === "d")
        toggleLooping(false);
    if (e.data[0] === "o")
        setLCDPowerState(true);
    if (e.data[0] === "p")
        setLCDPowerState(false);
};
connection.onclose = function () {
    console.log('WebSocket connection closed');
};

function setIntervalValue(value) {
    document.getElementById('loopingIntervalValue').value = value;
    //console.log('IntervalValue got set to: ' + value);
}
function setBrightnessValue(value) {
    document.getElementById('brightnessValue').value = value;
    //console.log('BrightnessValue got set to: ' + value);
}

function sendBrightnessValueOnInput() {
    var brightnessValue = document.getElementById('brightnessValue').value;
    console.log('Sending BrightnessValue: ' + brightnessValue);
    connection.send('b' + brightnessValue);
}

function sendLoopingIntervalValueOnInput() {
    var loopingIntervalValue = document.getElementById('loopingIntervalValue').value;
    console.log('Sending LoopingIntervalValue: ' + loopingIntervalValue);
    connection.send('i' + loopingIntervalValue);
}

function updateStyles() {
    if (LCDPower) {
        document.getElementById('brightnessValue').style.backgroundColor = "aquamarine";
        document.getElementById('LCDPowerToggle').checked = true;
    } else {
        document.getElementById('brightnessValue').style.backgroundColor = "transparent";
        document.getElementById('LCDPowerToggle').checked = false;
    }

    if (loopingEnableValue) {
        document.getElementById('brightnessValue').disabled = true;
        document.getElementById('brightnessValue').className = 'disabled';
        document.getElementById('loopingEffectToggle').checked = true;
        document.getElementById('loopingIntervalValue').style.display = 'block';
        document.getElementById('loopingIntervalValue').className = 'enabled';
        document.getElementById('loopingIntervalValue').disabled = false;
        document.getElementById('loopingIntervalValueLable').style.display = 'block';
        document.getElementById('loopingIntervalValueLable').disabled = false;
        document.getElementById('loopingIntervalValueLable').className = 'enabled';
    } else {
        document.getElementById('brightnessValue').disabled = false;
        document.getElementById('brightnessValue').className = 'enabled';
        document.getElementById('loopingEffectToggle').checked = false;
        document.getElementById('loopingIntervalValue').style.display = 'none';
        document.getElementById('loopingIntervalValue').className = 'disabled';
        document.getElementById('loopingIntervalValue').disabled = true;
        document.getElementById('loopingIntervalValueLable').style.display = 'none';
        document.getElementById('loopingIntervalValueLable').className = 'disabled';
        document.getElementById('loopingIntervalValueLable').disabled = true;
    }

}

function LCDPowerToggleOnClick(e) {
    e = e || window.event;
    var targ = e.target || e.srcElement;
    if (targ.nodeType == 3) targ = targ.parentNode; // defeat Safari bug
    connection.send((targ.checked ? "o" : "p"));
    setLCDPowerState(targ.checked);
    console.log("Set LCD power state to" + (targ.checked ? "on " : "off ") + "!");
}

function setLCDPowerState(powerState) {
    LCDPower = powerState;
    updateStyles();
}


function loopingEffectToggleOnClick(e) {
    e = e || window.event;
    var targ = e.target || e.srcElement;
    if (targ.nodeType == 3) targ = targ.parentNode; // defeat Safari bug
    connection.send((targ.checked ? "e" : "d"));
    toggleLooping(targ.checked);
    console.log("Turned " + (targ.checked ? "on " : "off ") + "looping effect!");
}

function toggleLooping(toggleValue) {
    loopingEnableValue = toggleValue;
    updateStyles();
}
