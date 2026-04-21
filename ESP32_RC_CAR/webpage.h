#pragma once
#include <Arduino.h>

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>ESP32 RC Car</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: Arial, sans-serif; text-align: center; background: #111; color: white; margin: 0; padding: 0; overscroll-behavior: none; }
  h2 { margin: 10px 0; }
  #camStream { width: 90%; max-width: 420px; border-radius: 10px; border: 2px solid #444; min-height: 150px; background: #222; }

  .controls-bar { display: flex; justify-content: center; align-items: center; gap: 10px; flex-wrap: wrap; margin: 6px 4px; }

  .btn { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; font-size: 16px; cursor: pointer; border-radius: 5px; }
  .btn-off { background-color: #f44336; }

  /* Toggle sport mode */
  .toggle-wrap { display: flex; align-items: center; gap: 8px; font-size: 14px; color: #ccc; }
  .toggle-track {
    position: relative; width: 46px; height: 26px;
    background: #555; border-radius: 13px;
    transition: background .2s; cursor: pointer;
  }
  .toggle-track.active { background: #ff9800; }
  .toggle-knob {
    position: absolute; top: 3px; left: 3px;
    width: 20px; height: 20px; background: white;
    border-radius: 50%; transition: transform .2s;
  }
  .toggle-track.active .toggle-knob { transform: translateX(20px); }
  .sport-label { font-weight: bold; color: #ff9800; }

  #joystick-zone {
    width: 300px; height: 300px;
    background: #333; border-radius: 50%;
    margin: 16px auto; position: relative;
    box-shadow: inset 0 0 20px rgba(0,0,0,0.6);
    touch-action: none;
  }
  #joystick-stick {
    width: 90px; height: 90px;
    background: #2196F3; border-radius: 50%;
    position: absolute; top: 105px; left: 105px;
    box-shadow: 0 0 10px rgba(0,0,0,0.5);
    transition: transform 0.05s linear;
  }
  .info-box { margin: 6px 0 10px; font-size: 14px; color: #aaa; }
</style>
</head>
<body>

<h2>ESP32 RC Car</h2>

<div>
  <img id="camStream" src="/stream"><br>
</div>

<div class="controls-bar">
  <button id="camBtn" class="btn btn-off" onclick="toggleCamera()">Wyłącz kamerę</button>

  <!-- Przycisk trybu sportowego -->
  <div class="toggle-wrap">
    <span>Auto</span>
    <div id="sportTrack" class="toggle-track active" onclick="toggleSportMode()">
      <div class="toggle-knob"></div>
    </div>
    <span class="sport-label" id="sportLabel"> SPORT</span>
  </div>
</div>

<div id="joystick-zone">
  <div id="joystick-stick"></div>
</div>

<div class="info-box">
  Skręt: <span id="angleVal">90</span> &nbsp;|&nbsp; Moc: <span id="speedVal">0</span>
</div>

<script>
  let websocket;
  const gateway = `ws://${window.location.hostname}:81/`;
  let camEnabled = true;
  let sportMode = true; // domyślnie włączony — zgodnie z applySportMode(true) w setup()

  window.addEventListener('load', () => {
    initWebSocket();
    initJoystick();
  });

  function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onclose = () => { setTimeout(initWebSocket, 2000); };
  }

  function toggleCamera() {
    const img = document.getElementById("camStream");
    const btn = document.getElementById("camBtn");
    camEnabled = !camEnabled;
    if (camEnabled) {
      img.src = "/stream"; img.style.display = "inline";
      btn.innerText = "Wyłącz kamerę"; btn.className = "btn btn-off";
    } else {
      img.src = ""; img.style.display = "none";
      btn.innerText = "Włącz kamerę"; btn.className = "btn";
    }
  }

  function toggleSportMode() {
    sportMode = !sportMode;
    const track = document.getElementById("sportTrack");
    const label = document.getElementById("sportLabel");

    if (sportMode) {
      track.classList.add("active");
      label.textContent = "🏎 SPORT";
      label.style.color = "#ff9800";
    } else {
      track.classList.remove("active");
      label.textContent = "AUTO";
      label.style.color = "#aaa";
    }

    if (websocket && websocket.readyState === WebSocket.OPEN) {
      websocket.send(JSON.stringify({ sportMode: sportMode }));
    }
  }

  function initJoystick() {
    const zone = document.getElementById('joystick-zone');
    const stick = document.getElementById('joystick-stick');
    const maxDiff = 105;
    let active = false;
    let lastSendTime = 0;
    let lastAngle = 90;
    let lastSpeed = 0;
    let keepaliveTimer = null;

    function handleStart(e) {
      active = true;
      // Keepalive: co 200ms wysyłaj aktualną pozycję nawet gdy się nie zmienia.
      // TIMEOUT_MS na ESP32 = 500ms — wysyłamy 2.5x częściej, więc failsafe nigdy nie odpali.
      keepaliveTimer = setInterval(() => {
        if (active) sendData(lastAngle, lastSpeed, true);
      }, 200);
      moveStick(e);
    }
    function handleEnd(e) {
      active = false;
      clearInterval(keepaliveTimer);
      stick.style.transform = `translate(0px, 0px)`;
      sendData(90, 0, true);
    }

    function moveStick(e) {
      if (!active) return;
      e.preventDefault();

      let clientX = e.touches ? e.touches[0].clientX : e.clientX;
      let clientY = e.touches ? e.touches[0].clientY : e.clientY;
      let rect = zone.getBoundingClientRect();

      let dx = clientX - (rect.left + rect.width / 2);
      let dy = clientY - (rect.top + rect.height / 2);
      let distance = Math.sqrt(dx*dx + dy*dy);

      if (distance > maxDiff) {
        dx = dx * (maxDiff / distance);
        dy = dy * (maxDiff / distance);
      }

      stick.style.transform = `translate(${dx}px, ${dy}px)`;

      let angle = Math.round(((dx + maxDiff) / (maxDiff * 2)) * 180);
      let speed = Math.round(((-dy + maxDiff) / (maxDiff * 2)) * 510 - 255);

      sendData(angle, speed, false);
    }

    zone.addEventListener('touchstart', handleStart, {passive: false});
    document.addEventListener('touchmove', moveStick, {passive: false});
    document.addEventListener('touchend', handleEnd);
    zone.addEventListener('mousedown', handleStart);
    document.addEventListener('mousemove', moveStick);
    document.addEventListener('mouseup', handleEnd);

    function sendData(angle, speed, force) {
      document.getElementById('angleVal').innerText = angle;
      document.getElementById('speedVal').innerText = speed;

      let now = Date.now();
      let changed = (angle !== lastAngle || speed !== lastSpeed);

      if (!force) {
        if (!changed && (now - lastSendTime < 150)) return;
        if (changed && (now - lastSendTime < 50)) return;
      }

      lastSendTime = now;
      lastAngle = angle;
      lastSpeed = speed;

      if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({ steering: angle, speed: speed }));
      }
    }
  }
</script>
</body>
</html>
)rawliteral";
