#define BLYNK_TEMPLATE_ID   "TMPL2dAfvB-7p"
#define BLYNK_TEMPLATE_NAME "miniprojet"
#define BLYNK_AUTH_TOKEN    "Hl1JgFIQ1Sfz01Bwa6fjBJsh9y5ulLbd"

/*#define BLYNK_TEMPLATE_ID "TMPL2lwIBnk0g"
#define BLYNK_TEMPLATE_NAME "miniprojet"
#define BLYNK_AUTH_TOKEN "cbRrcVqWe_W4zyO7j0hmaFMT4ws7KziP"
*/
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <Adafruit_TCS34725.h>
#include <DHT.h>

// WiFi 
char ssid[] = "Redmi 9A"; 
char pass[] = "Killua2011";

// Virtual Pins
#define VP_SERVO_H   V0
#define VP_SERVO_V   V1
#define VP_DVERT     V2
#define VP_DHORIZ    V3
#define VP_COLOR_C   V4
#define VP_RED_RATIO V5
#define VP_CURRENT   V6
#define VP_TEMP      V7  
#define VP_HUMID     V8 
#define poussiere    V9

// Capteur de couleur TCS34725 
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Capteur de courant ACS712 
#define ACS_PIN A0
const float sensitivity = 0.066;
int zeroOffset = 0;

// Capteur DHT11 
#define DHTPIN 2       
#define DHTTYPE DHT11   
DHT dht(DHTPIN,DHTTYPE);

// Variables pour la communication avec Arduino 
char buffer[50];
int indexBuffer = 0;
unsigned long previousMillis = 0;
const unsigned long interval = 1000; // 1 seconde

int latest_dVert = 0;
int latest_dHoriz = 0;

float dustThreshold = 1.00;

void setup() {
  Serial.begin(9600);

  // Connexion Blynk 
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialisation du capteur de couleur 
  if (!tcs.begin()) Serial.println("Color sensor not found!");
  else Serial.println("TCS34725 ready");

  // Calibration du capteur de courant 
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(ACS_PIN);
    delay(10);
  }
  zeroOffset = sum / 100;
  Serial.print("Zero offset = "); Serial.println(zeroOffset);

  // Initialisation du DHT11
  dht.begin();
  delay(1000); 
}

void loop() {
  Blynk.run();
  unsigned long currentMillis = millis();

  // Lecture et mise à jour toutes les "interval"
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Demande de données à Arduino 
    Serial.print('R');

    // Lecture des données envoyées par Arduino
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        buffer[indexBuffer] = '\0';
        indexBuffer = 0;
        String msg = String(buffer);
        msg.trim();

        if (msg.startsWith("<A>,")) {
          msg.replace("<A>,", "");

          int comma1 = msg.indexOf(',');
          int comma2 = msg.indexOf(',', comma1 + 1);
          int comma3 = msg.indexOf(',', comma2 + 1);

          int servoH = msg.substring(0, comma1).toInt();
          int servoV = msg.substring(comma1 + 1, comma2).toInt();
          int dVert  = msg.substring(comma2 + 1, comma3).toInt();
          int dHoriz = msg.substring(comma3 + 1).toInt();

          //  Envoi vers Blynk 
          Blynk.virtualWrite(VP_SERVO_H, servoH);
          Blynk.virtualWrite(VP_SERVO_V, servoV);
          Blynk.virtualWrite(VP_DVERT, dVert);
          Blynk.virtualWrite(VP_DHORIZ, dHoriz);

          latest_dVert = dVert;
          latest_dHoriz = dHoriz;

          // Lecture capteur de couleur
          uint16_t r, g, b, c;
          tcs.getRawData(&r, &g, &b, &c);
          float redRatio = (c > 0) ? ((float)r / c) : 0.0;
          if (r / c > dustThreshold) {
            Serial.println("Il y a de la poussière !");
            Blynk.virtualWrite(poussiere, "Il y a de la poussière !"); // envoyer texte à Blynk
          } else {
            Serial.println("Pas de poussière !");
            Blynk.virtualWrite(poussiere, "Pas de poussière");
          }
          // Lecture capteur de courant 
          int raw = analogRead(ACS_PIN);
          float voltage = raw * 5.0 / 1023.0;
          float vc = zeroOffset * 5.0 / 1023.0;
          float currentA = (vc - voltage) / sensitivity;

          // Lecture DHT11 
          float h = dht.readHumidity(); 
          float t = dht.readTemperature();
          if (isnan(h) || isnan(t)) {
            Serial.println("Failed to read DHT11! Retrying...");
            delay(2000); 
            h = dht.readHumidity();
            t = dht.readTemperature();
          } 
          if (!isnan(h) && !isnan(t)) {
            Blynk.virtualWrite(VP_TEMP, t);
            Blynk.virtualWrite(VP_HUMID, h);
            Serial.print("Temp: ");
            Serial.print(t);
            Serial.print(" °C  | Humidity: ");
            Serial.print(h);
            Serial.println(" %");
          } else {
            Serial.println("Failed to read DHT11!");
          }

          // Envoi données capteurs vers Blynk 
          Blynk.virtualWrite(VP_COLOR_C, c);
          Blynk.virtualWrite(VP_RED_RATIO, redRatio);
          Blynk.virtualWrite(VP_CURRENT, currentA);

          // Debug Serial
          Serial.print("[ARDUINO] ");
          Serial.print("H="); Serial.print(servoH);
          Serial.print(" V="); Serial.print(servoV);
          Serial.print(" dV="); Serial.print(dVert);
          Serial.print(" dH="); Serial.print(dHoriz);
          Serial.print(" | Color C="); Serial.print(c);
          Serial.print(" r/c="); Serial.print(redRatio);
          Serial.print(" | Current="); Serial.println(currentA, 3);
        }
      } else {
        if (indexBuffer < 49) buffer[indexBuffer++] = c;
        else indexBuffer = 0;
      }
    }

    // Mise à jour dernières valeurs pour Blynk 
    Blynk.virtualWrite(VP_DVERT, latest_dVert);
    Blynk.virtualWrite(VP_DHORIZ, latest_dHoriz);
  }
}



/*
#include <ACS712.h>
#include <Adafruit_TCS34725.h>

// --- Color sensor ---
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// --- ACS712 connections ---
#define ACS712_PIN A0
// (pin, Vref, ADC resolution, sensitivity mV/A)
ACS712 sensor(ACS712_PIN, 5.0, 1023, 66);

// --- Timing ---
unsigned long lastPrint = 0;
const unsigned long printInterval = 500;

unsigned long lastColorRead = 0;
const unsigned long colorInterval = 10;

// --- Voltage divider ( استخدميه فقط إذا عندك resistor divider )
const float R1 = 4700.0;
const float R2 = 1000.0;

void setup() {
  Serial.begin(115200);

  // Initialize color sensor
  if (!tcs.begin()) {
    Serial.println("Color sensor not found!");
  } else {
    Serial.println("TCS34725 found");
  }

  // Initialize ACS712 (إزالة الانحياز)

}

void loop() {
  unsigned long now = millis();

  // ---------------------------
  //  قراءة حساس الألوان
  // ---------------------------
  uint16_t r, g, b, c;

  if (now - lastColorRead >= colorInterval) {
    tcs.getRawData(&r, &g, &b, &c);
    lastColorRead = now;
  }

  // نسبة الأحمر = مؤشر غبار خفيف
  float redRatio = (c > 0) ? ((float) r / c) : 0.0;
  bool maybeDust = (redRatio > 1.5);

  // ---------------------------
  //  قراءة التيار من ACS712
  // ---------------------------
  float current_mA = sensor.mA_DC(1);
  float current_A  = current_mA / 1000.0;

  // ---------------------------
  //  طباعة
  // ---------------------------
  if (now - lastPrint >= printInterval) {

    Serial.print("Color C = "); Serial.print(c);
    Serial.print("  R/C = "); Serial.println(redRatio, 2);

    Serial.print("Current (mA) = ");
    Serial.println(current_mA, 3);

    if (maybeDust)
      Serial.println("Warning: possible dust detected!");

    Serial.println("-----------------------------");

    lastPrint = now;
  }

  delay(10);
}
*/




















