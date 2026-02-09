/* CHANGE LOG
 * v1.0 17/06/2020 15:40
 * - Foundational sketch created and working.
 * 
 * v1.1 17/06/2020 23:00
 * - Added error checking in initialisation
 * - Created a function makeRequest(byte,String) to send a HTTP request
 * - Added roadblocks in initialisation if an error occurs. The error is send as a notification.
 * 
 * v1.2 18/06/2020 01:00
 * - Added a getTime() function to receive the time when a client connects to the server.
 * - Added a calcTime() function to constantly keep track of time downto -1sec accuracy.
 * 
 * v1.3 18/06/2020 22:00
 * - Bug fixes
 * - Added a working automatic mode with a time scheduler.
 * - Added mode selection buttons to the website *need to finish*
 * - Changed accelerometer operating mode to accel-only, wake-up cycle of 5Hz
 * - During OFF or off-schedule (AUTO) modes the accelerometer goes to sleep
 */

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <Wire.h>
#include "index.h"
#include "settings.h"
#include "message.h"
#include <EEPROM_Rotate.h>

MPU6050 accel;
#define SDA 0
#define SCL 2

#define MA_SS 8
#define accDelay 200
#define httpDelay 10000
#define theInit 0
#define theAlert 1
#define theError 2

const char* yourSSID = "admin";
const char* yourPASS = "pass";
const char* defaultSSID = "ESP_Garage";
const char* defaultPASS = "NotSecure";

String eventInit = "ESP_Initialised";
String eventAlert = "ESP_GarageDoor";
String eventError = "ESP_Error";
String IFTTT_URL1 = "http://maker.ifttt.com/trigger/";
String IFTTT_URL2 = "/with/key/";
String IFTTT_KEY = "";
String IFTTT_URL;

int16_t ax, ay, az;
bool firstTime = false;
bool autoMode;
bool startSet;
bool endSet;
bool sysState;
bool timeInc = true;
byte minCount;
byte hrCount;
byte timeStart[2];
byte timeEnd[2];
unsigned long secCount;
float angle;
byte angle_thresh;
long angle_i = 0;
bool trigger_set;
bool trigger_pull;
bool master_trigger;
unsigned long timer;
unsigned long accTimer;
unsigned long httpTimer;
unsigned long timeTimer;
int httpCode;
int offset[6] = {-3057, -777, 1335, 109, -10, -54};
// OFFSET VALUES: -3057ax -777ay 1335az 109gx -10gy -54gz

// filter defines
unsigned long buff;
int ax_buff[MA_SS];
int ay_buff[MA_SS];
int az_buff[MA_SS];
long ax_f;
long ay_f;
long az_f;

HTTPClient http;
ESP8266WebServer server(80);
EEPROM_Rotate EEPROMr;

void setup() {
  /* ----- I2C ----- */
  delay(500);
  Wire.begin(SDA, SCL);

  Serial.begin(115200);
  Serial.println();

  EEPROMr.size(4);
  EEPROMr.begin(4096);

  /* ----- WiFi ----- */
  Serial.print("Connecting to local network");
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(yourSSID, yourPASS);
  timer = (unsigned long)millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (((unsigned long)millis() - timer) > 10000) {
      firstTime = true;
      Serial.println();
      Serial.println("Connection timed out!\n");
      Serial.println("Setting up a first-time AP...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(defaultSSID, defaultPASS);
    }
  }
  Serial.println();
  if (!firstTime) { Serial.println("Connected!");Serial.print("IP: ");Serial.println(WiFi.localIP()); }
  else {
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  /* ----- Server ----- */
  delay(1000);
  Serial.println();
  if (!firstTime) {
    server.on("/",[](){server.send_P(200,"text/html",webpage);});
    server.on("/settings",[](){server.send_P(200,"text/html",settings);});
    server.on("/submit",handleForm);
    server.on("/toggle",toggle);
    server.on("/update",updateWeb);
    server.on("/getMode",sendMode);
    server.on("/getSched",sendSched);
    server.on("/giveMode",receiveMode);
    server.on("/time",getTime);
  } else {
    server.on("/",[](){server.send_P(200,"text/html",settings);});
  }
  Serial.println("Setting up a server...");
  server.begin();
  
  /* ----- Accelerometer init ----- */
  if (firstTime) return;
  delay(1000);
  accel.reset();
  delay(250);
  Serial.println();
  Serial.println("Resetting and initialising the accel...\n");
  accel.initialize();
  setOffset();
  if (accel.testConnection()) {
    Serial.println("I2C: OK!");
  } else {
    Serial.println("I2C: ERROR! Check the I2C bus.\n");
    makeRequest(theError,"I2C: ERROR! Check the I2C bus.");
    ESP.reset();
  }

  /* ----- Accelerometer zeroing ----- */
  delay(1000);
  Serial.println();
  Serial.print("Filling buffer");
  for (byte i=0; i<255; i++) {
    getAccel();
    if (i%85 == 0) Serial.print(".");
    delay(1);
  }
  Serial.println();
  delay(250);
  Serial.print("Zeroing the accelerometer");
  for (int i=0; i<1000; i++) {
    getAccel();
    calcAngle();
    angle_i += angle;
    if (i%333 == 0) Serial.print(".");
    delay(1);
  }
  angle_i /= 1000;
  wakeMPU();
  //accel.setSleepEnabled(true);
  accel.setSleepEnabled(false);
  Serial.println("\n");

  /* ----- Inititialisation complete ----- */
  makeRequest(theInit,"");
  Serial.println();
  Serial.println("System Ready!");
  Serial.println();
  
  accTimer = httpTimer = millis();
  trigger_set = false;
  master_trigger = false;
  startSet = false;
  endSet = false;
  angle_thresh = 15;
  
  autoMode = 1;
  sysState = 0;
  timeStart[0] = 20;
  timeStart[1] = 0;
  timeEnd[0] = 7;
  timeEnd[1] = 0;
}

void loop() {
  server.handleClient();
  if (firstTime) return;
  calcTime();
  /*
  if (autoMode == 1) {
    if (minCount == timeStart[1] && hrCount == timeStart[0] && !startSet) {
      wakeMPU();
      sysState = 1;
      startSet = true;
      endSet = false;
      Serial.println("Schedule started.\n");
    } else if (minCount == timeEnd[1] && hrCount == timeEnd[0] && !endSet) {
      accel.setSleepEnabled(true);
      sysState = 0;
      startSet = false;
      endSet = true;
      Serial.println("Schedule ended.\n");
    }
  }
  */
  if (autoMode == 1) {
    if (minCount == timeStart[1] && hrCount == timeStart[0] && !startSet) {
      wakeMPU();
      sysState = 1;
      startSet = true;
      endSet = false;
      Serial.println("Schedule started.\n");
    } else if (minCount == timeEnd[1] && hrCount == timeEnd[0] && !endSet) {
      accel.setSleepEnabled(true);
      sysState = 0;
      startSet = false;
      endSet = true;
      Serial.println("Schedule ended.\n");
    }
  }
  
  if ((((unsigned long)millis() - accTimer) > accDelay) && sysState == 1) {
    getAccel();
    calcAngle();
    angle -= angle_i;
    if (angle > angle_thresh && !master_trigger) trigger_set = true;
    else if (angle < angle_thresh) master_trigger = false;
    accTimer = millis();
  }

  if ((((unsigned long)millis() - httpTimer) > httpDelay) && trigger_set) {
    makeRequest(theAlert,"");
    Serial.println();
    httpTimer = millis();
  }
}

void getAccel() {
  accel.getAcceleration(&ax, &ay, &az);
  // Moving avg filter
  if (buff < MA_SS) {
    ax_buff[buff] = ax;
    ay_buff[buff] = ay;
    az_buff[buff] = az;
    ax_f = ay_f = az_f = 0;
    for (byte i=0; i<MA_SS; i++) {
      ax_f += ax_buff[i];
      ay_f += ay_buff[i];
      az_f += az_buff[i];
    }
    ax_f /= MA_SS;
    ay_f /= MA_SS;
    az_f /= MA_SS;
    buff++;
    if (buff == MA_SS) buff = 0;
  }
}

void calcAngle() {
  angle = atan2(-az_f, ax_f);
  if (angle > PI/2) angle = PI - angle;
  if (angle < -PI/2) angle = -PI - angle;
  angle *= 180/PI;
}

void setOffset() {
  accel.setXAccelOffset(offset[0]);
  accel.setYAccelOffset(offset[1]);
  accel.setZAccelOffset(offset[2]);
  Serial.println("Sensor offsets are set!\n");
}

void toggle() {
  trigger_set = false;
  master_trigger = true;
  Serial.println("Standing by...");
  Serial.println();
}

void updateWeb() {
  if (!trigger_set) {
    if (autoMode) {
      if (startSet) server.send(200,"text/plain","AUTO ON-SCHEDULE");
      else server.send(200,"text/plain","AUTO OFF-SCHEDULE");
    } else {
      if (sysState) server.send(200,"text/plain","MANUAL ON");
      else server.send(200,"text/plain","MANUAL OFF");
    }
  } else server.send(200,"text/plain","INTRUDER!");
}

void makeRequest(byte event, String payload) {
  String URL = IFTTT_URL1 + (event==0?eventInit:event==1?eventAlert:eventError) + IFTTT_URL2 + IFTTT_KEY;
  static String pl1 = "{\"value1\":\"";
  static String pl2 = "\"}";
  String httpPayload = pl1 + payload + pl2;
  http.begin(URL);
  Serial.println(event==0?"Sending confirmation...":event==1?"Sending alert....":"Sending error at "+payload);
  if (event==2) {
    http.addHeader("Content-Type", "application/json"); 
    httpCode = http.POST(httpPayload);
  } else httpCode = http.GET();
  if (httpCode > 0) {
    Serial.print("HTTP Code: ");Serial.println(httpCode);
  } else Serial.println("HTTP ERROR!");
  http.end();
}

void getTime() {
  hrCount = server.arg("hour").toInt();
  minCount = server.arg("minute").toInt();
  secCount = server.arg("second").toInt();
  Serial.println("Time synchronised!\n");
  Serial.print("The time is: ");
  Serial.println((String)hrCount+":"+(String)minCount+":"+(String)secCount);
  secCount *= 1000;
  timeInc = true;
}

void calcTime() {
  if (timeInc) {
    timeTimer = millis();
    timeInc = false;
  }
  if (((unsigned long)millis() - timeTimer) >= (60000-secCount)) {
    secCount = 0;
    minCount++;
    if (minCount >= 60) {
      minCount = 0;
      hrCount++;
      hrCount = (hrCount >= 24?0:hrCount);
    }
    Serial.println((String)hrCount+":"+(String)minCount);
    timeInc = true;
  }
}

void wakeMPU() { // low-power 5Hz accelerometer-only mode.
  accel.setWakeCycleEnabled(true);
  accel.setWakeFrequency(1);
  accel.setSleepEnabled(false);
  accel.setTempSensorEnabled(false);
  accel.setStandbyXGyroEnabled(true);
  accel.setStandbyYGyroEnabled(true);
  accel.setStandbyZGyroEnabled(true);
}

void handleForm() {
  static String ssid;
  static String pass;
  static String startTime;
  static String endTime;
  static int delimiter;

  Serial.println("Form submitted!");

  ssid = server.arg("SSID");
  pass = server.arg("PASS");
  startTime = server.arg("startTime");
  endTime = server.arg("endTime");
  
  if (startTime != "") {
    delimiter = startTime.indexOf(':');
    timeStart[0] = startTime.substring(0,delimiter).toInt();
    timeStart[1] = startTime.substring(delimiter+1).toInt();
  }
  if (endTime != "") {
    delimiter = endTime.indexOf(':');
    timeEnd[0] = endTime.substring(0,delimiter).toInt();
    timeEnd[1] = endTime.substring(delimiter+1).toInt();
  }
  Serial.println("Schedule start: "+(String)timeStart[0]+":"+(String)timeStart[1]);
  Serial.println("Schedule end: "+(String)timeEnd[0]+":"+(String)timeEnd[1]);
  server.send_P(200,"text/html",message);

  // if ssid > 0 && pass > 0 then reset device
}

void sendMode() {
  Serial.println("Mode sent!");
  if (autoMode == 1) server.send(200,"text/plain","2");
  else if (sysState == 1) server.send(200,"text/plain","1");
  else server.send(200,"text/plain","0");
}

void sendSched() {
  Serial.println("Sched sent!");
  server.send(200,"text/plain",(String)timeStart[0]+":"+(String)timeStart[1]+"/"+(String)timeEnd[0]+":"+(String)timeEnd[1]);
}

void receiveMode() {
  static String newmode;
  newmode = server.arg("theMode");
  if (newmode.toInt() < 2) {
    autoMode = 0;
    sysState = newmode.toInt();
  } else {
    autoMode = 1;
    sysState = 0;
  }
  Serial.println("Mode updated!");
}

/* write to eeprom where mystr is char[]
 * for(int i=0 ; i < sizeof(mystr) ; i++)
  {
    EEPROM.write( i,mystr[i] );
  }
  EEPROM.write(sizeof(mystr)+1,'/0')
  EEPROM.commit();


  read from eeprom
  index = 0;
  this = EEPROM.read(address+offset);
  

  add so that on first boot the device sends an 'OK/0' (if not present) to EEPROM and writes 0 to SSID and PASS and 21->7 sched
  then on second boot, read from the offset "OK"[2+1] until /0 is found. that data is SSID, then read from the offset + length of SSID + 1 and so on
  data is sorted as such: SSID[32+1], PASS[32+1], timeStart[0][1+1], timeStart[1][1+1], timeEnd[0][1+1], timeEnd[1][1+1]. [=74+3]
  add char limits on website
 */

void writeEEPROMdefaults() {
  for (byte i=0; i<sizeof(yourSSID); i++) {
    EEPROMr.write(3+i, yourSSID[i]);
  }
  for (byte i=0; i<sizeof(yourPASS); i++) {
    EEPROMr.write(3+sizeof(yourSSID)+i, yourPASS[i]);
  }
  Serial.println("Defaults Written!");
}

char[] readEEPROM() {
  static char okCheck[2+1];
  static char ssid[];
  static char pass[];
  okCheck[0] = EEPROMr.read(0);
  okCheck[1] = EEPROMr.read(1);
  if (okCheck != "ok") {
    writeEEPROM(0,"ok");
    writeEEPROMdefaults();
  } else {
    
  }
}
