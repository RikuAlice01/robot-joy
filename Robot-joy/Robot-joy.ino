// RikuAlice01 – Gamepad Servo Edition
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

/* ================= WIFI ================= */
const char* ssid = "Robot-00";
const char* password = "12345678";

/* ================= MOTOR ================= */
#define IN1 D0
#define IN2 D1
#define IN3 D2
#define IN4 D3

/* ================= SERVO ================= */
#define PIN_A D5   // GPIO14
#define PIN_B D6   // GPIO12
#define PIN_C D7   // GPIO13
#define PIN_D D8   // GPIO15

Servo servoA, servoB, servoC, servoD;
int servoAngle[4] = {0,0,0,0};
bool servoState[4] = {false,false,false,false};
unsigned long servoTime[4] = {0,0,0,0};
const unsigned long SERVO_DELAY = 300;

/* ================= SERVER ================= */
ESP8266WebServer server(80);

/* ================= SAFETY ================= */
unsigned long lastCmdTime = 0;
const unsigned long CMD_TIMEOUT = 600;

/* ================= LED ================= */
unsigned long ledTimer = 0;
bool ledState = false;

/* ================= MOTOR ================= */
void motorStop(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}

/* ================= SERVO ================= */
void toggleServo(uint8_t id){
  if(millis() - servoTime[id] < SERVO_DELAY) return;
  servoTime[id] = millis();

  servoState[id] = !servoState[id];
  servoAngle[id] = servoState[id] ? 90 : 0;

  switch(id){
    case 0: servoA.write(servoAngle[id]); break;
    case 1: servoB.write(servoAngle[id]); break;
    case 2: servoC.write(servoAngle[id]); break;
    case 3: servoD.write(servoAngle[id]); break;
  }

  Serial.print("Servo "); Serial.print(id);
  Serial.print(" -> "); Serial.println(servoAngle[id]);
}

/* ================= SETUP ================= */
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  motorStop();

  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);

  servoA.attach(PIN_A);
  servoB.attach(PIN_B);
  servoC.attach(PIN_C);
  servoD.attach(PIN_D);

  servoA.write(0); servoB.write(0);
  servoC.write(0); servoD.write(0);

  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,1,1),gw(192,168,1,1),sn(255,255,255,0);
  WiFi.softAPConfig(ip,gw,sn);
  WiFi.softAP(ssid,password);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/",handleRoot);
  server.on("/cmd",handleCMD);
  server.begin();
}

/* ================= LOOP ================= */
void loop(){
  server.handleClient();

  if(millis()-lastCmdTime > CMD_TIMEOUT){
    motorStop();
  }

  int c = WiFi.softAPgetStationNum();
  if(c>0){
    digitalWrite(LED_BUILTIN,LOW);
  }else{
    if(millis()-ledTimer>500){
      ledTimer=millis();
      ledState=!ledState;
      digitalWrite(LED_BUILTIN,ledState?LOW:HIGH);
    }
  }
}

/* ================= CMD ================= */
void handleCMD(){
  if(!server.hasArg("v")){
    server.send(200,"text/plain","NO CMD");
    return;
  }

  String cmd = server.arg("v");
  Serial.print("CMD from browser: ");
  Serial.println(cmd);
  lastCmdTime = millis();

  // ----- MOTOR -----
  if(cmd=="F"){
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  }
  else if(cmd=="B"){
    digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
    digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  }
  else if(cmd=="L"){
    digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  }
  else if(cmd=="R"){
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
    digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  }
  else if(cmd=="S"){
    motorStop();
  }

  // ----- SERVO -----
else if(cmd=="A1"){ toggleServo(0); server.send(200,"text/plain","DONE"); return; }
else if(cmd=="A2"){ toggleServo(1); server.send(200,"text/plain","DONE"); return; }
else if(cmd=="A3"){ toggleServo(2); server.send(200,"text/plain","DONE"); return; }
else if(cmd=="A4"){ toggleServo(3); server.send(200,"text/plain","DONE"); return; }


  // status sync
  else if(cmd=="STATE"){
    String s="";
    for(int i=0;i<4;i++) s += servoState[i] ? "1":"0";
    server.send(200,"text/plain",s);
    return;
  }

  server.send(200,"text/plain","OK");
}

/* ================= WEB UI ================= */
void handleRoot(){
String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{user-select:none;-webkit-user-select:none;}
body{
  margin:0;background:#0f1115;color:#fff;
  font-family:Arial;height:100vh;
  display:flex;justify-content:center;align-items:center;
}
.main{
  display:flex;gap:40px;
}
:root{
  --b:clamp(70px,20vw,100px);
  --g:clamp(10px,3vw,18px);
}
.pad,.act{
  display:grid;
  grid-template:repeat(3,var(--b))/repeat(3,var(--b));
  gap:var(--g);
}
button{
  border:none;border-radius:50%;
  font-size:22px;font-weight:bold;
  color:#000;
}
.pad button{
  background:#444;color:#fff;
}
.y{background:#f1c40f;}
.x{background:#3498db;}
.b{background:#e74c3c;}
.a{background:#2ecc71;}
.on{
  box-shadow:0 0 20px rgba(255,255,255,.9);
  filter:brightness(1.3);
}
@media (orientation:portrait){
  .main{flex-direction:column;}
}
</style>

<script>
let t=null,cmd="";
let servoLocked=false;
let servoState=[0,0,0,0];

function vibrate(){
  if(navigator.vibrate) navigator.vibrate(30);
}

function send(v){
  return fetch("/cmd?v="+v).then(r=>r.text());
}

// ----- movement -----
function down(v){
  cmd=v;
  send(v);
  t=setInterval(()=>send(cmd),120);
}
function up(){
  if(t){clearInterval(t);t=null;}
  send("S");
}

// ----- servo action (SAFE) -----
async function servoPress(id,cmd){
  if(servoLocked) return;   
  servoLocked = true;

  vibrate();

  try{
    const res = await send(cmd); 
    if(res.trim() === "DONE"){
      servoState[id] = !servoState[id];
      updateUI();
    }
  }catch(e){
    console.log("ERR",e);
  }

  servoLocked = false; 
}

function updateUI(){
  ["y","x","b","a"].forEach((c,i)=>{
    document.querySelector("."+c)
      .classList.toggle("on",servoState[i]);
  });
}
</script>

</head>

<body>
<div class="main">

<div class="pad">
<div></div>
<button onpointerdown="down('F')" onpointerup="up()">UP</button>
<div></div>
<button onpointerdown="down('L')" onpointerup="up()">LEFT</button>
<div></div>
<button onpointerdown="down('R')" onpointerup="up()">RIGHT</button>
<div></div>
<button onpointerdown="down('B')" onpointerup="up()">DOWN</button>
<div></div>
</div>

<div class="act">
<div></div>
<button class="y" onpointerdown="servoPress(0,'A1')">Y</button>
<div></div>
<button class="x" onpointerdown="servoPress(1,'A2')">X</button>
<div></div>
<button class="b" onpointerdown="servoPress(2,'A3')">B</button>
<div></div>
<button class="a" onpointerdown="servoPress(3,'A4')">A</button>
</div>

</div>
</body>
</html>
)rawliteral";

server.send(200,"text/html",html);
}
