// RikuAlice01 – WiFi Robot Gamepad FINAL v2
// โปรแกรมควบคุมหุ่นยนต์ผ่าน WiFi โดยใช้ ESP8266 เป็น Access Point
// มีหน้าเว็บเป็นจอยเกมแพด + ปุ่ม action (Y,X,B,A)

#include <ESP8266WiFi.h>          // ไลบรารี WiFi สำหรับ ESP8266
#include <ESP8266WebServer.h>     // ไลบรารีสร้าง Web Server บน ESP8266
#include <Servo.h>                // ไลบรารีควบคุมเซอร์โว

/* ================= WIFI ================= */
// ตั้งค่า WiFi hotspot ที่หุ่นยนต์จะสร้างเอง
const char* ssid     = "Robot-00";     // ชื่อ WiFi
const char* password = "12345678";     // รหัสผ่าน (ต้องพิมพ์ให้ถูก)

/* ================= MOTOR ================= */
// ขา pin ที่ต่อกับมอเตอร์ driver (เช่น L298N)
#define IN1 D0     // มอเตอร์ซ้าย - ช่อง IN1
#define IN2 D1     // มอเตอร์ซ้าย - ช่อง IN2
#define IN3 D2     // มอเตอร์ขวา  - ช่อง IN1
#define IN4 D3     // มอเตอร์ขวา  - ช่อง IN2

/* ================= SERVO ================= */
#define SERVO_PIN D5   // ขาที่ต่อเซอร์โว (ปากคีบ/แขน/หัว/ฯลฯ)

/* ================= OUTPUT ================= */
// ขาอื่น ๆ ที่ใช้เป็น output ทั่วไป
#define OUT_A2   D6      // ปุ่ม X → output ธรรมดา (ติด/ดับ)
#define OUT_A3   D7      // ปุ่ม B → output ธรรมดา (ติด/ดับ)
#define BUZZER   D8      // ปุ่ม A → buzzer (เสียง)

/* ================= OBJECT ================= */
// สร้าง object เพื่อใช้งาน
ESP8266WebServer server(80);   // Web server ฟังที่ port 80
Servo servo;                   // ตัวควบคุมเซอร์โว

/* ================= ACTION CONTROL ================= */
// ตัวแปรสถานะการทำงาน (ป้องกันการส่งคำสั่งทับซ้อน)
bool busy = false;             // กำลังทำงานอยู่หรือไม่
String pendingCmd = "";        // คำสั่งที่รอคิว (ถ้ากำลัง busy อยู่)
unsigned long actionStart = 0;     // เวลาเริ่มทำ action (ms)
unsigned long actionDuration = 0;  // ระยะเวลาที่ควรทำ action นี้ (ms)
bool servoState = false;           // สถานะเซอร์โว (true=90°, false=0°)

/* ================= LED ================= */
// ใช้ LED_BUILTIN แสดงสถานะการเชื่อมต่อ
unsigned long ledTimer = 0;
bool ledState = false;

/* ================= MOTOR ================= */
void motorStop(){
  // ปิดมอเตอร์ทั้งหมด (ทุกขาเป็น LOW)
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}

void motorRun(String cmd){
  // ควบคุมทิศทางตามคำสั่ง F B L R
  if(cmd=="F"){          // หน้า
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  }
  else if(cmd=="B"){     // ถอยหลัง
    digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
    digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  }
  else if(cmd=="L"){     // เลี้ยวซ้าย (ขวาหน้า ซ้ายถอย)
    digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH);
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  }
  else if(cmd=="R"){     // เลี้ยวขวา
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
    digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH);
  }
}

/* ================= START ACTION ================= */
void startAction(String cmd){
  // เริ่มทำงานคำสั่งใหม่ → ตั้ง busy = true
  busy = true;
  actionStart = millis();       // บันทึกเวลาเริ่ม

  if(cmd=="F"||cmd=="B"||cmd=="L"||cmd=="R"){
    // การเคลื่อนที่ → วิ่ง 200 ms แล้วหยุดเอง
    actionDuration = 200;
    motorRun(cmd);
  }
  else if(cmd=="A1"){           // ปุ่ม Y → ควบคุมเซอร์โว toggle 0° ↔ 90°
    actionDuration = 100;
    servoState = !servoState;
    servo.write(servoState ? 90 : 0);
  }
  else if(cmd=="A4"){           // ปุ่ม A → buzzer เปิดสั้น ๆ 100 ms
    actionDuration = 100;
    digitalWrite(BUZZER, HIGH);
  }

  // แสดงว่ากำลังรันคำสั่งอะไร (ดูใน Serial Monitor)
  Serial.print("EXEC: ");
  Serial.println(cmd);
}

/* ================= SETUP ================= */
void setup(){
  Serial.begin(115200);         // เปิด Serial เพื่อ debug

  // ตั้งค่า pin มอเตอร์เป็น OUTPUT
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  motorStop();                  // มอเตอร์หยุดก่อน

  // pin อื่น ๆ
  pinMode(OUT_A2,OUTPUT);
  pinMode(OUT_A3,OUTPUT);
  pinMode(BUZZER,OUTPUT);

  digitalWrite(OUT_A2,LOW);
  digitalWrite(OUT_A3,LOW);
  digitalWrite(BUZZER,LOW);

  // LED บนบอร์ด (ใช้แสดงสถานะเชื่อมต่อ)
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);   // เริ่มต้น = ดับ (HIGH = ดับใน ESP8266 บางรุ่น)

  // เซอร์โว
  servo.attach(SERVO_PIN);
  servo.write(0);               // เริ่มต้นที่ 0 องศา

  // ตั้งค่าเป็น Access Point (ไม่มีเชื่อมต่อ WiFi ภายนอก)
  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,1,1), gw(192,168,1,1), sn(255,255,255,0);
  WiFi.softAPConfig(ip, gw, sn);
  WiFi.softAP(ssid, password, 1, 0, 1);  // max_connection = 1

  // ตั้งค่าเส้นทางเว็บ
  server.on("/", handleRoot);       // หน้าแรก → แสดงจอยแพด
  server.on("/cmd", handleCMD);     // รับคำสั่งจากปุ่ม
  server.on("/manifest.json", handleManifest);
  server.begin();                   // เริ่ม web server
}

/* ================= LOOP ================= */
void loop(){
  server.handleClient();      // รับ request จาก browser / โทรศัพท์

  // ตรวจสอบว่าคำสั่งหมดเวลาแล้วหรือยัง
  if(busy && millis()-actionStart >= actionDuration){
    motorStop();              // หยุดมอเตอร์
    digitalWrite(BUZZER,LOW); // ปิด buzzer
    busy = false;

    // ถ้ามีคำสั่งรอคิวอยู่ → ทำต่อเลย
    if(pendingCmd!=""){
      String c = pendingCmd;
      pendingCmd="";
      startAction(c);
    }
  }

  // แสดงสถานะ LED
  int c = WiFi.softAPgetStationNum();   // จำนวนเครื่องที่เชื่อมต่อ
  if(c > 0){
    digitalWrite(LED_BUILTIN, LOW);     // มีคนเชื่อมต่อ → LED ติด
  } else {
    // กระพริบทุก 500 ms เมื่อไม่มีใครเชื่อมต่อ
    if(millis()-ledTimer > 500){
      ledTimer = millis();
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
    }
  }
}

void handleManifest() {
  server.send(200, "application/json", R"rawliteral(
{
  "name": "Robot Gamepad",
  "short_name": "RobotPad",
  "start_url": "/",
  "display": "fullscreen",
  "background_color": "#0f1115",
  "theme_color": "#3498db",
  "orientation": "any",
  "icons": [
    {
      "src": "https://via.placeholder.com/192?text=RP",
      "sizes": "192x192",
      "type": "image/png"
    },
    {
      "src": "https://via.placeholder.com/512?text=RobotPad",
      "sizes": "512x512",
      "type": "image/png"
    }
  ]
}
)rawliteral");
}

/* ================= CMD ================= */
void handleCMD(){
  // รับ parameter ชื่อ v จาก URL เช่น /cmd?v=F
  if(!server.hasArg("v")){
    server.send(200,"text/plain","NO CMD");
    return;
  }

  String cmd = server.arg("v");

  // A2 และ A3 เป็นแบบกดค้าง (hold) ไม่ต้องรอ busy
  if(cmd=="A2_ON")     { digitalWrite(OUT_A2,HIGH); }
  else if(cmd=="A2_OFF"){ digitalWrite(OUT_A2,LOW);  }
  else if(cmd=="A3_ON") { digitalWrite(OUT_A3,HIGH); }
  else if(cmd=="A3_OFF"){ digitalWrite(OUT_A3,LOW);  }
  else {
    // คำสั่งอื่น ๆ → ถ้ากำลัง busy อยู่ ให้ใส่คิวรอ
    if(busy){
      pendingCmd = cmd;
      server.send(200,"text/plain","BUSY");
      return;
    }
    // ถ้าว่าง → เริ่มทำเลย
    startAction(cmd);
  }

  server.send(200,"text/plain","OK");
}

/* ================= WEB UI ================= */
void handleRoot(){
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <!-- ทำให้เป็น PWA -->
  <link rel="manifest" href="/manifest.json">
  <meta name="theme-color" content="#0f1115">
  <meta name="background-color" content="#0f1115">
  <title>Robot Gamepad</title>

  <style>
    * {
      user-select: none;
      -webkit-user-select: none;
      -webkit-tap-highlight-color: transparent; /* ลบ highlight สีน้ำเงินตอนกดบนมือถือ */
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      margin: 0;
      background: #0f1115;
      color: #fff;
      font-family: Arial, sans-serif;
      height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      overflow: hidden;
      touch-action: manipulation; /* ปรับปรุงการสัมผัส */
    }
    .main {
      display: flex;
      gap: 40px;
    }
    :root {
      --b: clamp(70px, 20vw, 100px);
      --g: clamp(10px, 3vw, 18px);
    }
    .pad, .act {
      display: grid;
      grid-template: repeat(3, var(--b)) / repeat(3, var(--b));
      gap: var(--g);
    }
    button {
      border: none;
      border-radius: 50%;
      font-size: 28px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.12s ease-out; /* เร็วหน่อยให้รู้สึก responsive */
      box-shadow: 0 6px 12px rgba(0,0,0,0.5);
      position: relative;
      overflow: hidden;
    }

    /* Hover effect: ขยาย + เรืองแสงเบา */
    button:hover {
      transform: scale(1.08);
      box-shadow: 0 10px 20px rgba(0,0,0,0.6),
                  0 0 20px rgba(255,255,255,0.15);
    }

    /* Active effect: กดแล้วยุบ + เรืองแสงแรง */
    button:active {
      transform: scale(0.92);
      box-shadow: 0 2px 6px rgba(0,0,0,0.7) inset,
                  0 0 30px rgba(255,255,255,0.4);
      filter: brightness(1.1);
    }

    /* สีปุ่มต่าง ๆ */
    .pad button {
      background: #444;
      color: #fff;
    }
    .y { background: #f1c40f; }
    .x { background: #3498db; }
    .b { background: #e74c3c; }
    .a { background: #2ecc71; }

    @media (orientation: portrait) {
      .main { flex-direction: column; gap: 30px; }
    }
  </style>

  <script>
    // ส่งคำสั่ง
    function send(v) {
      fetch("/cmd?v=" + v).catch(() => {});
      if (navigator.vibrate) navigator.vibrate(30);
    }

    // สำหรับ PWA - บังคับ fullscreen ถ้าเป็น standalone
    if (window.matchMedia('(display-mode: standalone)').matches || window.navigator.standalone === true) {
      document.documentElement.requestFullscreen?.();
    }
  </script>
</head>

<body>
<div class="main">

  <div class="pad">
    <div></div>
    <button onclick="send('F')">&#9650;</button>
    <div></div>
    <button onclick="send('L')">&#9664;</button>
    <div></div>
    <button onclick="send('R')">&#9654;</button>
    <div></div>
    <button onclick="send('B')">&#9660;</button>
    <div></div>
  </div>

  <div class="act">
    <div></div>
    <button class="y" onclick="send('A1')">Y</button>
    <div></div>
    <button class="x" onclick="send('A2')">X</button>
    <div></div>
    <button class="b" onclick="send('A3')">B</button>
    <div></div>
    <button class="a" onclick="send('A4')">A</button>
  </div>

</div>
</body>
</html>
)rawliteral");
}