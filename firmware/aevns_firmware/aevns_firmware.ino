#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiUDP.h>

// ── Motor Pins ────────────────────────────────────────────
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 32;
const int ENA = 33;
const int ENB = 14;

const int PWM_FREQ       = 1000;
const int PWM_RESOLUTION = 8;

// ── Ultrasonic Pins ───────────────────────────────────────
const int ECHO_LEFT  = 18;
const int TRIG_LEFT  = 5;
const int ECHO_RIGHT = 19;
const int TRIG_RIGHT = 22;
const int ECHO_FRONT = 21;
const int TRIG_FRONT = 23;

// ── LED ───────────────────────────────────────────────────
const int LED = 2;

// ── BMS Pins ──────────────────────────────────────────────
const int SDA_PIN  = 13;
const int SCL_PIN  = 15;
const int TEMP_PIN = 4;

// ── Speed Constants ───────────────────────────────────────
const int min_speed    = 145;
const int slow_speed   = 155;
const int normal_speed = 200;
const int fast_speed   = 250;

// ── BMS Thresholds ────────────────────────────────────────
const int MAX_TEMP    = 45;
const int LOW_BATTERY = 20;
const int CRIT_BATTERY = 10;

// ── BMS Global State ──────────────────────────────────────
float g_voltage       = 0;
float g_current       = 0;
float g_power         = 0;
float g_soc           = 100;
float g_temp          = 25;
float g_solar_voltage = 0;
float g_solar_current = 0;
bool  g_overTemp      = false;
bool  g_lowBattery    = false;
bool  g_critBattery   = false;
bool  g_solarActive   = false;

float g_left_d = 0;
float g_right_d = 0; 
float g_front_d = 0;

String g_left_zone = "CLEAR";
String g_right_zone = "CLEAR";
String g_front_zone = "CLEAR";

String g_action = "IDLE";
int g_drive_speed = 0;

// ── Networking ────────────────────────────────────────────────
const char* ssid = "Ali Khan";
const char* password = "06070809";
const char* laptopIP = "192.168.1.14";
const int port = 4210; 

// ── Timing ────────────────────────────────────────────────
unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long lastBMS_read = 0;
const unsigned long BMS_interval = 2000;
unsigned long lastWifiSend = 0;
const unsigned long WiFi_interval = 2000; 

// ── Sensor Objects ────────────────────────────────────────
Adafruit_INA219 battery_ina219(0x40);
Adafruit_INA219 solar_ina219(0x41);
OneWire one_wire(TEMP_PIN);
DallasTemperature tempSensor(&one_wire);
WiFiUDP udp;


// ── Distance ──────────────────────────────────────────────
float distance(int TRIG, int ECHO)
{
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long duration = pulseIn(ECHO, HIGH, 30000);
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2;
}

// ── Zone ──────────────────────────────────────────────────
String ZONE(float dist)
{
  if (dist > 0 && dist <= 30)  return "CLOSE";
  if (dist > 30 && dist <= 80) return "MEDIUM";
  return "CLEAR";
}

// ── LED ───────────────────────────────────────────────────
void updateLED(String L, String R, String F)
{
  if (L == "CLOSE" || R == "CLOSE" || F == "CLOSE")
  {
    digitalWrite(LED, HIGH);
    ledState = true;
  }
  else if (L == "MEDIUM" || R == "MEDIUM" || F == "MEDIUM")
  {
    unsigned long now = millis();
    if (now - lastBlinkTime >= 200)
    {
      ledState = !ledState;
      digitalWrite(LED, ledState);
      lastBlinkTime = now;
    }
  }
  else
  {
    digitalWrite(LED, LOW);
    ledState = false;
  }
}

// ── BMS ───────────────────────────────────────────────────
void readBMS()
{
  unsigned long now = millis();
  if (now - lastBMS_read < BMS_interval) return;
  lastBMS_read = now;

  // Battery
  g_voltage = battery_ina219.getBusVoltage_V();
  g_current = battery_ina219.getCurrent_mA();
  g_power   = battery_ina219.getPower_mW();
  g_soc     = constrain(((g_voltage - 6.0) / (8.4 - 6.0)) * 100.0, 0.0, 100.0);

  // Solar
  g_solar_voltage = solar_ina219.getBusVoltage_V();
  g_solar_current = solar_ina219.getCurrent_mA();
  g_solarActive   = (g_solar_voltage > 4.5 && g_solar_current > 5.0);

  // Temperature
  tempSensor.requestTemperatures();
  g_temp = tempSensor.getTempCByIndex(0);

  if (g_temp == DEVICE_DISCONNECTED_C)
  {
    Serial.println("ERROR: Temperature sensor disconnected");
    g_overTemp = true;
  }
  else
  {
    g_overTemp = g_temp > MAX_TEMP;
  }

  g_lowBattery  = g_soc < LOW_BATTERY;
  g_critBattery = g_soc < CRIT_BATTERY;

  Serial.println("===== BMS =====");
  Serial.print("Battery Voltage : "); Serial.print(g_voltage, 2);       Serial.println(" V");
  Serial.print("Battery Current : "); Serial.print(g_current, 2);       Serial.println(" mA");
  Serial.print("Battery Power   : "); Serial.print(g_power, 2);         Serial.println(" mW");
  Serial.print("SOC             : "); Serial.print(g_soc, 1);           Serial.println(" %");
  Serial.print("Temperature     : "); Serial.print(g_temp, 1);          Serial.println(" C");
  Serial.print("Solar Voltage   : "); Serial.print(g_solar_voltage, 2); Serial.println(" V");
  Serial.print("Solar Current   : "); Serial.print(g_solar_current, 2); Serial.println(" mA");
  Serial.print("Solar Active    : "); Serial.println(g_solarActive ? "YES" : "NO");
  if (g_overTemp)    Serial.println("WARNING : BATTERY OVERHEATING");
  if (g_critBattery) Serial.println("CRITICAL: BATTERY CRITICALLY LOW");
  else if (g_lowBattery) Serial.println("WARNING : LOW BATTERY");
  Serial.println("===============");
}

// ── Motors ────────────────────────────────────────────────
void leftMotorForward()  { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
void leftMotorBackward() { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
void rightMotorForward() { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
void rightMotorBackward(){ digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }

void leftMotorStop()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  ledcWrite(ENA, 255);
}

void rightMotorStop()
{
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENB, 255);
}

void setMotorSpeed(int left_speed, int right_speed)
{
  ledcWrite(ENA, constrain(left_speed,  0, 255));
  ledcWrite(ENB, constrain(right_speed, 0, 255));
}

void moveForward(int left_speed, int right_speed)
{
  leftMotorForward();
  rightMotorForward();
  setMotorSpeed(left_speed, right_speed);
}

void moveBackward(int left_speed, int right_speed)
{
  leftMotorBackward();
  rightMotorBackward();
  setMotorSpeed(left_speed, right_speed);
}

void turnLeft(int right_speed)
{
  leftMotorStop();
  rightMotorForward();
  ledcWrite(ENB, constrain(right_speed, 0, 255));
}

void turnRight(int left_speed)
{
  rightMotorStop();
  leftMotorForward();
  ledcWrite(ENA, constrain(left_speed, 0, 255));
}

void stopBothMotors()
{
  leftMotorStop();
  rightMotorStop();
}

void correctionDifferentSpeed(int left_speed, int right_speed)
{
  leftMotorForward();
  rightMotorForward();
  setMotorSpeed(left_speed, right_speed);
}

// ── Navigation ────────────────────────────────────────────
void navigate()
{
  if (g_overTemp || g_critBattery)
  {
    stopBothMotors();
 if (g_overTemp)
        Serial.println("BMS HALT: TEMPERATURE PROBLEM");

    if (g_critBattery)
        Serial.println("BMS HALT: CRITICAL BATTERY");

    g_action = "BMS HALT";
    g_drive_speed = 0;

    delay(500);
    return;
  }

  int driveSpeed;
  int turnSpeed;
  if (g_lowBattery == true) {
    driveSpeed = slow_speed;
  } else {
    driveSpeed = fast_speed;
  }
  if (g_lowBattery == true) {
    turnSpeed = slow_speed;
  } else {
    turnSpeed = normal_speed;
  }

  float LEFT_D  = distance(TRIG_LEFT,  ECHO_LEFT);  delay(60);
  float RIGHT_D = distance(TRIG_RIGHT, ECHO_RIGHT); delay(60);
  float FRONT_D = distance(TRIG_FRONT, ECHO_FRONT); delay(60);

  g_left_d = LEFT_D;
  g_right_d = RIGHT_D;
  g_front_d = FRONT_D;


  String LEFT_ZONE  = ZONE(LEFT_D);
  String RIGHT_ZONE = ZONE(RIGHT_D);
  String FRONT_ZONE = ZONE(FRONT_D);

  g_left_zone = LEFT_ZONE;
  g_right_zone = RIGHT_ZONE;
  g_front_zone = FRONT_ZONE;

  Serial.print("Left: ");  Serial.print(LEFT_D);  Serial.print(" cm | "); Serial.println(LEFT_ZONE);
  Serial.print("Right: "); Serial.print(RIGHT_D); Serial.print(" cm | "); Serial.println(RIGHT_ZONE);
  Serial.print("Front: "); Serial.print(FRONT_D); Serial.print(" cm | "); Serial.println(FRONT_ZONE);

  updateLED(LEFT_ZONE, RIGHT_ZONE, FRONT_ZONE);

  if (FRONT_ZONE == "CLEAR")
  {
    if (LEFT_D < 15)
    {
      Serial.println("ACTION: CORRECTING RIGHT");
      g_action = "CORRECTING RIGHT";
      g_drive_speed = driveSpeed;
      correctionDifferentSpeed(driveSpeed, min_speed);
      delay(200);
      
    }
    else if (RIGHT_D < 15)
    {
      Serial.println("ACTION: CORRECTING LEFT");
      g_action = "CORRECTING LEFT";
      g_drive_speed = driveSpeed;
      correctionDifferentSpeed(min_speed, driveSpeed);
      delay(200);
      
    }
    else
    {
      Serial.println("ACTION: MOVING FORWARD");
      g_action = "MOVING FORWARD";
      g_drive_speed = driveSpeed;
      moveForward(driveSpeed, driveSpeed);
      delay(400);
      
      
    }
  }
  else if (FRONT_ZONE == "MEDIUM")
  {
    Serial.println("ACTION: SLOWING DOWN");
    g_action = "SLOWING DOWN";
    g_drive_speed = slow_speed;
    moveForward(slow_speed, slow_speed);
    delay(600);
  }
  else
  {
    Serial.println("FRONT BLOCKED");
    g_action = "FRONT BLOCKED";
    g_drive_speed = 0;
    stopBothMotors();
    delay(100);

    if (RIGHT_D > LEFT_D + 10)
    {
      Serial.println("ACTION: TURN RIGHT");
      g_action = "TURN RIGHT";
      g_drive_speed = turnSpeed;
      turnRight(turnSpeed);
      delay(450);
      stopBothMotors();
    }
    else if (LEFT_D > RIGHT_D + 10)
    {
      Serial.println("ACTION: TURN LEFT");
      g_action = "TURN LEFT";
      g_drive_speed = turnSpeed;
      turnLeft(turnSpeed);
      delay(450);
      stopBothMotors();
    }
    else
    {
      Serial.println("ACTION: REVERSING");
      g_action = "REVERSING";
      g_drive_speed = normal_speed;
      stopBothMotors();
      delay(100);
      moveBackward(normal_speed, normal_speed);
      delay(600);
      stopBothMotors();
      delay(200);

      float new_RIGHT_D = distance(TRIG_RIGHT, ECHO_RIGHT); delay(60);
      float new_LEFT_D  = distance(TRIG_LEFT,  ECHO_LEFT);  delay(60);

      g_right_d = new_RIGHT_D;
      g_left_d = new_LEFT_D;
      g_right_zone = ZONE(new_RIGHT_D);
      g_left_zone = ZONE(new_LEFT_D);

      if (new_RIGHT_D > new_LEFT_D)
      {
        Serial.println("ACTION: TURN RIGHT AFTER REVERSING");
        g_action = "TURN RIGHT AFTER REVERSING";
        g_drive_speed = turnSpeed;
        turnRight(turnSpeed);
        delay(450);
        stopBothMotors();
      }
      else
      {
        Serial.println("ACTION: TURN LEFT AFTER REVERSING");
        g_action = "TURN LEFT AFTER REVERSING";
        g_drive_speed = turnSpeed;
        turnLeft(turnSpeed);
        delay(450);
        stopBothMotors();
      }
    }
  }
}

// ── Setup ─────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_LEFT,  OUTPUT); pinMode(ECHO_LEFT,  INPUT);
  pinMode(TRIG_RIGHT, OUTPUT); pinMode(ECHO_RIGHT, INPUT);
  pinMode(TRIG_FRONT, OUTPUT); pinMode(ECHO_FRONT, INPUT);
  digitalWrite(TRIG_LEFT,  LOW);
  digitalWrite(TRIG_RIGHT, LOW);
  digitalWrite(TRIG_FRONT, LOW);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);

  ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);
  setMotorSpeed(0, 0);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!battery_ina219.begin(&Wire))
  {
    Serial.println("Battery INA219 not found at 0x40");
    while (1);
  }
  Serial.println("Battery INA219 ready");

  if (!solar_ina219.begin(&Wire))
  {
    Serial.println("Solar INA219 not found at 0x41");
    while (1);
  }
  Serial.println("Solar INA219 ready");

  tempSensor.begin();
  Serial.println("Temperature Sensor Ready");
  Serial.println("AEVNS Ready");

  WiFi.begin(ssid, password);

  while (WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.println(WiFi.localIP());
  udp.begin(port);
}

void SendData(){
  unsigned long now = millis();
  if (now - lastWifiSend < WiFi_interval) return;
  lastWifiSend = now;

  String message = String(g_voltage) + "," +
                String(g_current) + "," +
                String(g_power) + "," +
                String(g_soc) + "," +  
                String(g_temp) + "," +
                String(g_solar_voltage) + "," +
                String(g_solar_current) + "," +
                String(g_solarActive ? "YES" : "NO") + "," +
                String(g_left_d) + "," +
                String(g_right_d) + "," +
                String(g_front_d) + "," +
                String(g_left_zone) + "," +
                String(g_right_zone) + "," +
                String(g_front_zone) + "," +
                String(g_action) + "," +
                String(g_drive_speed);

  udp.beginPacket(laptopIP,port);
  udp.print(message);
  udp.endPacket();
}

// ── Loop ──────────────────────────────────────────────────
void loop()
{
  readBMS();
  navigate();
  SendData();
}