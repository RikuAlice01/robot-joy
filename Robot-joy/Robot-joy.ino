//RikuAlice01
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/* ========= WIFI ========= */
const char* ssid = "Robot-00";
const char* password = "12345678";

/* ========= MOTOR PIN ========= */
#define IN1 D5  // GPIO14
#define IN2 D6  // GPIO12

/* ========= ACTION PIN ========= */
#define PIN_A D1  // GPIO5
#define PIN_B D2  // GPIO4
#define PIN_C D7  // GPIO13
#define PIN_D D8  // GPIO15

ESP8266WebServer server(80);

/* ========= FUNCTION ========= */
void motorStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

/* ========= SETUP ========= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_D, OUTPUT);

  motorStop();
  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  digitalWrite(PIN_C, LOW);
  digitalWrite(PIN_D, LOW);

  WiFi.mode(WIFI_AP);

  // ตั้งค่า IP เอง
  IPAddress local_ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // ปล่อย WiFi
  WiFi.softAP(ssid, password);

  Serial.println("=== WiFi AP Started ===");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP  : ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/cmd", handleCMD);

  server.begin();
}

/* ========= LOOP ========= */
void loop() {
  server.handleClient();
}

/* ========= HANDLE CMD ========= */
void handleCMD() {
  if (!server.hasArg("v")) {
    server.send(200, "text/plain", "NO CMD");
    return;
  }

  String cmd = server.arg("v");

  // ===== LOG =====
  Serial.print("CMD from browser: ");
  Serial.println(cmd);

  // ----- MOTOR -----
  if (cmd == "F") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (cmd == "B") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else if (cmd == "L" || cmd == "R" || cmd == "S") {
    motorStop();
  }

  // ----- ACTION BUTTON -----
  else if (cmd == "A_ON")
    digitalWrite(PIN_A, HIGH);
  else if (cmd == "A_OFF") digitalWrite(PIN_A, LOW);

  else if (cmd == "B_ON") digitalWrite(PIN_B, HIGH);
  else if (cmd == "B_OFF") digitalWrite(PIN_B, LOW);

  else if (cmd == "C_ON") digitalWrite(PIN_C, HIGH);
  else if (cmd == "C_OFF") digitalWrite(PIN_C, LOW);

  else if (cmd == "D_ON") digitalWrite(PIN_D, HIGH);
  else if (cmd == "D_OFF") digitalWrite(PIN_D, LOW);

  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
*{
  -webkit-user-select:none;
  user-select:none;
  -webkit-touch-callout:none;
}

body{
  margin:0;
  background:#111;
  color:#fff;
  font-family:Arial;
  height:100vh;
  display:flex;
  justify-content:center;
  align-items:center;
}

.main{
  display:flex;
  width:100%;
  height:100%;
  max-width:900px;
  justify-content:space-around;
  align-items:center;
}

/* ---------- AUTO SIZE ---------- */
:root{
  --btn: clamp(64px, 18vw, 96px);
  --gap: clamp(8px, 3vw, 16px);
}

/* ---------- D PAD ---------- */
.pad{
  display:grid;
  grid-template-columns:var(--btn) var(--btn) var(--btn);
  grid-template-rows:var(--btn) var(--btn) var(--btn);
  gap:var(--gap);
}

.pad button{
  width:var(--btn);
  height:var(--btn);
  border-radius:50%;
  border:none;
  background:#444;
  color:#fff;
  font-size:clamp(14px,4vw,18px);
}

/* ---------- ACTION (XBOX) ---------- */
.action{
  display:grid;
  grid-template-columns:var(--btn) var(--btn) var(--btn);
  grid-template-rows:var(--btn) var(--btn) var(--btn);
  gap:var(--gap);
}

.action button{
  width:var(--btn);
  height:var(--btn);
  border-radius:50%;
  border:none;
  font-size:clamp(18px,5vw,26px);
  font-weight:bold;
  color:#000;
}

/* Xbox Colors */
.btnY{ background:#f1c40f; grid-column:2; grid-row:1; }
.btnX{ background:#3498db; grid-column:1; grid-row:2; }
.btnB{ background:#e74c3c; grid-column:3; grid-row:2; }
.btnA{ background:#2ecc71; grid-column:2; grid-row:3; }

button:active{
  filter:brightness(1.3);
}

/* ---------- PORTRAIT ---------- */
@media (orientation: portrait){
  .main{
    flex-direction:column;
    justify-content:space-evenly;
  }
}
</style>

<script>
function vibrate(){
  if (navigator.vibrate) navigator.vibrate(20);
}

function send(v){
  fetch("/cmd?v="+v);
}

function press(v,e){
  e.preventDefault();
  vibrate();
  send(v);
}

function release(v,e){
  e.preventDefault();
  send(v);
}
</script>
</head>

<body>

<div class="main">

<!-- D PAD -->
<div class="pad">
  <div></div>
  <button onpointerdown="press('F',event)"
          onpointerup="release('S',event)"
          onpointercancel="release('S',event)">UP</button>
  <div></div>

  <button onpointerdown="press('L',event)"
          onpointerup="release('S',event)"
          onpointercancel="release('S',event)">LEFT</button>
  <div></div>
  <button onpointerdown="press('R',event)"
          onpointerup="release('S',event)"
          onpointercancel="release('S',event)">RIGHT</button>

  <div></div>
  <button onpointerdown="press('B',event)"
          onpointerup="release('S',event)"
          onpointercancel="release('S',event)">DOWN</button>
  <div></div>
</div>

<!-- ACTION -->
<div class="action">
  <button class="btnY"
          onpointerdown="press('A_ON',event)"
          onpointerup="release('A_OFF',event)"
          onpointercancel="release('A_OFF',event)">Y</button>

  <button class="btnX"
          onpointerdown="press('B_ON',event)"
          onpointerup="release('B_OFF',event)"
          onpointercancel="release('B_OFF',event)">X</button>

  <button class="btnB"
          onpointerdown="press('C_ON',event)"
          onpointerup="release('C_OFF',event)"
          onpointercancel="release('C_OFF',event)">B</button>

  <button class="btnA"
          onpointerdown="press('D_ON',event)"
          onpointerup="release('D_OFF',event)"
          onpointercancel="release('D_OFF',event)">A</button>
</div>

</div>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}
