/* CHANGE LOG
 * v1.0 17/06/2020 15:40
 * - Foundational sketch created and working.
 */

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <Wire.h>
#include "index.h"

MPU6050 accel;
#define SDA 0
#define SCL 2

#define MA_SS 8
#define accDelay 75
#define httpDelay 10000
#define theInit 0
#define theAlert 1
#define theError 2


const char* yourSSID = "admin";
const char* yourPASS = "pass";

String eventInit = "ESP_Initialised";
String eventAlert = "ESP_GarageDoor";
String eventError = "ESP_Error";
String IFTTT_URL1 = "http://maker.ifttt.com/trigger/";
String IFTTT_URL2 = "/with/key/";
String IFTTT_KEY = "";
String IFTTT_URL;

int16_t ax, ay, az;
byte errorCheck = 0;
float angle;
long angle_i = 0;
bool trigger_set;
bool trigger_pull;
unsigned long timer;
unsigned long accTimer;
unsigned long httpTimer;
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

void setup() {
  /* ----- I2C ----- */
  delay(500);
  Wire.begin(SDA, SCL);

  Serial.begin(115200);
  Serial.println();

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
      Serial.println();
      Serial.println("Connection timed out!\n");
      makeRequest(theError,"WiFi");
      while(true) delay(0);
    }
  }
  Serial.println();
  Serial.println("Connected!");Serial.print("IP: ");Serial.println(WiFi.localIP());
  errorCheck |= B00000001;

  /* ----- Server ----- */
  delay(1000);
  Serial.println();
  server.on("/",[](){server.send_P(200,"text/html",webpage);});
  server.on("/toggle",toggle);
  server.on("/update",updateWeb);
  server.on("/time",getTime);
  Serial.println("Setting up a server...");
  server.begin();
  errorCheck |= B00000010;

  /* ----- Accelerometer init ----- */
  delay(1000);
  Serial.println();
  Serial.println("Initiliasing the accelerometer.");
  accel.initialize();

  // verify connection
  if (accel.testConnection()) {
    Serial.println("I2C: OK!");
    errorCheck |= B00000100;
  } else {
    Serial.println("I2C: ERROR! Check the I2C bus.\n");
    makeRequest(theError,"I2C Test");
    while (true) delay(0);
  }

  /* ----- Offsets ----- */
  delay(1000);
  Serial.println();
  setOffset();

  /* ----- Accelerometer zeroing ----- */
  delay(1000);
  Serial.println();
  Serial.print("Filling buffer");
  for (byte i=0; i<255; i++) {
    accel.getAcceleration(&ax, &ay, &az);
    MAF();
    if (i%26 == 0) Serial.print(".");
    delay(1);
  }
  Serial.println();
  delay(250);
  Serial.print("Zeroing the accelerometer");
  for (int i=0; i<1000; i++) {
    accel.getAcceleration(&ax, &ay, &az);
    MAF();
    calcAngle();
    angle_i += angle;
    if (i%100 == 0) Serial.print(".");
    delay(1);
  }
  angle_i /= 1000;
  errorCheck |= B00001000;
  Serial.println("\n");

  /* ----- Init error check ----- */
  if (errorCheck == B00001111) {
    makeRequest(theInit,"");
    Serial.println();
    Serial.println("System Ready!");
    Serial.println();
  } else {
    Serial.println("The system could not initialise!\n");
    makeRequest(theError, "initilisation");
    while (true) delay(0);
  }
  
  accTimer = httpTimer = millis();
  trigger_set = false;
}

void loop() {
  server.handleClient();
  
  if (((unsigned long)millis() - accTimer) > accDelay) {
    accel.getAcceleration(&ax, &ay, &az);
    MAF();
    calcAngle();
    angle -= angle_i;
    if (angle > 20) trigger_set = true;
    accTimer = millis();
  }

  if ((((unsigned long)millis() - httpTimer) > httpDelay) && trigger_set) {
    makeRequest(theAlert,"");
    Serial.println();
    httpTimer = millis();
  }
}

void MAF() {
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
  /*
  accel.setXGyroOffset(offset[3]);
  accel.setYGyroOffset(offset[4]);
  accel.setZGyroOffset(offset[5]);
  */
  Serial.println("Sensor offsets are set!");
}

void toggle() {
  trigger_set = false;
  Serial.println("Standing by...");
  Serial.println();
}

void updateWeb() {
  if (!trigger_set) server.send(200,"text/plain","STANDBY");
  else server.send(200,"text/plain","INTRUDER!");
}

String concatUrl(byte event) {
  String URL = IFTTT_URL1 + (event==0?eventInit:event==1?eventAlert:eventError) + IFTTT_URL2 + IFTTT_KEY;
  return URL;
}

void makeRequest(byte event, String payload) {
  String URL = IFTTT_URL1 + (event==0?eventInit:event==1?eventAlert:eventError) + IFTTT_URL2 + IFTTT_KEY;
  String pl1 = "{\"value1\":\"";
  String pl2 = "\"}";
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
  
}
/*
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, yourSSID);
  EEPROM.get(0+sizeof(yourSSID), yourPASS);
  char ok[2+1];
  EEPROM.get(0+sizeof(yourSSID)+sizeof(yourPASS), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(yourSSID);
  Serial.println(strlen(yourPASS)>0?"********":"<no password>");
}

void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, yourSSID);
  EEPROM.put(0+sizeof(yourSSID), yourPASS);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(yourSSID)+sizeof(yourPASS), ok);
  EEPROM.commit();
  EEPROM.end();
}
*/
