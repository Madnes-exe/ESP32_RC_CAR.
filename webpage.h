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
  body { font-family: Arial, sans-serif; text-align: center; background: #111; color: white; margin: 0; padding: 0; overscroll-behavior: none; }
  h2 { margin: 10px 0; }
  #camStream { width: 90%; max-width: 420px; border-radius: 10px; border: 2px solid #444; min-height: 150px; background: #222; }
  .btn { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; font-size: 16px; margin: 10px 2px; cursor: pointer; border-radius: 5px; }
  .btn-off { background-color: #f44336; }
  
  #joystick-zone { 
    width: 360px; 
    height: 360px; 
    background: #333; 
    border-radius: 50%; 
    margin: 20px auto; 
    position: relative; 
    box-shadow: inset 0 0 20px rgba(0,0,0,0.6); 
    touch-action: none; 
  }
  #joystick-stick { 
    width: 100px; 
    height: 100px; 
    background: #2196F3; 
    border-radius: 50%; 
    position: absolute; 
    top: 130px; 
    left: 130px; 
    box-shadow: 0 0 10px rgba(0,0,0,0.5); 
    transition: transform 0.05s linear; 
  }
  .info-box { margin-top: 10px; font-size: 16px; color: #aaa; }
</style>
</head>
<body>

<h2>ESP32 RC Car</h2>
<div>
  <img id="camStream" src="/stream"><br>
  <button id="camBtn" class="btn btn-off" onclick="toggleCamera()">Wyłącz kamerę</button>
</div>

<div id="joystick-zone">
  <div id="joystick-stick"></div>
</div>

<div class="info-box">
  Skręt: <span id="angleVal">90</span> | Moc: <span id="speedVal">0</span>
</div>

<script>
  let websocket;
  const gateway = `ws://${window.location.hostname}:81/`;
  let camEnabled = true;

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

  function initJoystick() {
    const zone = document.getElementById('joystick-zone');
    const stick = document.getElementById('joystick-stick');
    const maxDiff = 130; 
    let active = false;
    let lastSendTime = 0;
    
    let lastAngle = 90;
    let lastSpeed = 0;

    function handleStart(e) { active = true; moveStick(e); }
    function handleEnd(e) {
      active = false;
      stick.style.transform = `translate(0px, 0px)`;
      sendData(90, 0, true); 
    }
    
    function moveStick(e) {
      if (!active) return;
      e.preventDefault();
      
      let clientX = e.touches ? e.touches[0].clientX : e.clientX;
      let clientY = e.touches ? e.touches[0].clientY : e.clientY;
      let rect = zone.getBoundingClientRect();
      
      let centerX = rect.left + rect.width / 2;
      let centerY = rect.top + rect.height / 2;
      
      let dx = clientX - centerX;
      let dy = clientY - centerY;
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
        if (!changed && (now - lastSendTime < 300)) return;
        if (changed && (now - lastSendTime < 50)) return;
      }

      lastSendTime = now;
      lastAngle = angle;
      lastSpeed = speed;

      if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({ steering: angle, speed: speed }));
      }
    }
  }
</script>
</body>
</html>
)rawliteral";