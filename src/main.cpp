#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define SERVO_PIN 13

Servo servo;
WebServer server(80);

const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Servo Tester</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui;background:#111;color:#eee;display:flex;justify-content:center;align-items:center;min-height:100vh}
.card{background:#1a1a2e;border-radius:16px;padding:32px;width:min(400px,90vw);text-align:center;box-shadow:0 8px 32px rgba(0,0,0,.5)}
h1{font-size:1.4rem;margin-bottom:24px;color:#7c83ff}
.angle-display{font-size:3rem;font-weight:700;color:#fff;margin:16px 0}
.angle-display span{color:#7c83ff}
input[type=range]{-webkit-appearance:none;width:100%;height:8px;border-radius:4px;background:#333;outline:none;margin:16px 0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:#7c83ff;cursor:pointer}
.presets{display:flex;gap:8px;margin-top:20px;justify-content:center;flex-wrap:wrap}
.presets button{background:#2a2a4a;color:#eee;border:none;padding:10px 18px;border-radius:8px;cursor:pointer;font-size:.9rem;transition:background .2s}
.presets button:hover{background:#7c83ff}
.sweep{margin-top:16px}
.sweep button{background:#4a3a6a;color:#eee;border:none;padding:10px 24px;border-radius:8px;cursor:pointer;font-size:.9rem}
.sweep button:hover{background:#7c83ff}
.section{margin-top:24px;border-top:1px solid #333;padding-top:20px}
.section h2{font-size:1rem;color:#7c83ff;margin-bottom:12px}
.angle-inputs{display:flex;gap:12px;justify-content:center;align-items:center;margin-bottom:12px}
.angle-inputs label{font-size:.85rem;color:#aaa}
.angle-inputs input[type=number]{width:70px;background:#222;border:1px solid #444;color:#eee;padding:8px;border-radius:6px;text-align:center;font-size:1rem}
.press-btn{background:#ff4a6a;color:#fff;border:none;padding:14px 32px;border-radius:10px;cursor:pointer;font-size:1.1rem;font-weight:700;transition:background .2s,transform .1s}
.press-btn:hover{background:#ff6a85}
.press-btn:active{transform:scale(0.95)}
.press-row{display:flex;gap:8px;justify-content:center;align-items:center;margin-top:8px;flex-wrap:wrap}
.press-row label{font-size:.85rem;color:#aaa}
.press-row input[type=number]{width:70px;background:#222;border:1px solid #444;color:#eee;padding:6px;border-radius:6px;text-align:center;font-size:.9rem}
.press-row span{font-size:.85rem;color:#aaa}
.press-btn.long{background:#7c4aff}
.press-btn.long:hover{background:#9a6aff}
.status{margin-top:16px;font-size:.8rem;color:#666}
</style>
</head>
<body>
<div class="card">
<h1>Servo Motor Tester</h1>
<div class="angle-display"><span id="val">90</span>&deg;</div>
<input type="range" id="slider" min="0" max="180" value="90">
<div class="presets">
<button onclick="set(0)">0&deg;</button>
<button onclick="set(45)">45&deg;</button>
<button onclick="set(90)">90&deg;</button>
<button onclick="set(135)">135&deg;</button>
<button onclick="set(180)">180&deg;</button>
</div>
<div class="sweep">
<button onclick="sweep()">Sweep</button>
</div>
<div class="section">
<h2>Button Press</h2>
<div class="angle-inputs">
<div><label>Rest</label><br><input type="number" id="restAngle" min="0" max="180" value="90"></div>
<div><label>Press</label><br><input type="number" id="pressAngle" min="0" max="180" value="45"></div>
</div>
<div class="press-row">
<label>Short hold</label>
<input type="number" id="shortHold" min="10" max="1000" value="100">
<span>ms</span>
</div>
<div class="press-row">
<label>Long hold</label>
<input type="number" id="longHold" min="1000" max="10000" value="3000" step="100">
<span>ms</span>
</div>
<div style="display:flex;gap:12px;justify-content:center;margin-top:16px">
<button class="press-btn" onclick="doPress('short')">SHORT</button>
<button class="press-btn long" onclick="doPress('long')">LONG</button>
</div>
</div>
<div class="status" id="status">Connected</div>
</div>
<script>
const slider=document.getElementById('slider');
const val=document.getElementById('val');
const status=document.getElementById('status');
let sweeping=false;

slider.addEventListener('input',()=>{
  val.textContent=slider.value;
  send(slider.value);
});

function set(v){slider.value=v;val.textContent=v;send(v);}

function send(angle){
  fetch('/set?angle='+angle)
    .then(r=>r.ok?status.textContent='OK':status.textContent='Error')
    .catch(()=>status.textContent='Connection lost');
}

function sweep(){
  if(sweeping)return;
  sweeping=true;
  let a=0,dir=1;
  const iv=setInterval(()=>{
    a+=dir*2;
    if(a>=180)dir=-1;
    if(a<=0){clearInterval(iv);sweeping=false;}
    slider.value=a;val.textContent=a;send(a);
  },30);
}

function doPress(type){
  const rest=document.getElementById('restAngle').value;
  const pa=document.getElementById('pressAngle').value;
  const hold=type==='long'?document.getElementById('longHold').value:document.getElementById('shortHold').value;
  fetch('/press?rest='+rest+'&press='+pa+'&hold='+hold)
    .then(r=>r.ok?status.textContent=type+' press!':status.textContent='Error')
    .catch(()=>status.textContent='Connection lost');
}
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleSet() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    angle = constrain(angle, 0, 180);
    servo.write(angle);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing angle");
  }
}

void handlePress() {
  if (server.hasArg("rest") && server.hasArg("press") && server.hasArg("hold")) {
    int restAngle = constrain(server.arg("rest").toInt(), 0, 180);
    int pressAngle = constrain(server.arg("press").toInt(), 0, 180);
    int holdTime = constrain(server.arg("hold").toInt(), 10, 10000);
    servo.write(restAngle);
    delay(200);
    servo.write(pressAngle);
    delay(holdTime);
    servo.write(restAngle);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing params");
  }
}

void setup() {
  Serial.begin(115200);

  servo.attach(SERVO_PIN);
  servo.write(90);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/press", handlePress);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}
