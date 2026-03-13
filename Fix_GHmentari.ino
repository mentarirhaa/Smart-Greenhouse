/*************************************************************
   SMART GREENHOUSE 
 *************************************************************/

#define BLYNK_TEMPLATE_ID    "TMPL6XfmA6xjo"
#define BLYNK_TEMPLATE_NAME  "Greenhouse Monitoring"
#define BLYNK_AUTH_TOKEN     "q8VBQBWiBEBZvPv29LTGsVqTaJHP0TyY"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>

// WIFI CONFIG
char ssid[] = "Zet.Edu";
char pass[] = "tanyasaya";

// GOOGLE SHEETS URL
const char* scriptURL = "https://script.google.com/macros/s/AKfycbydInFEqvGfKINxcyUiqQlfXdzudvCqnb1JlR1YdMi8oWHL3wBz8_w1wZjAfvqywEmY/exec";

/* ================= PIN & OBJEK ================= */
#define DHTPIN     4
#define DHTTYPE    DHT22
#define FANPIN     18
#define LEDPIN     5
#define RELAY_PIN  17
#define SOIL_PIN   34
#define SDA_PIN    21
#define SCL_PIN    22

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
BlynkTimer timer;

/* ================= VARIABEL GLOBAL ================= */
float suhu, luxFiltered, soil, pumpSec;
int fanPercent, lampPWM;
const int pwmFreq = 5000;
const int pwmRes  = 8;

/* ================= FUZZY SUGENO LOGIC ================= */
float mu_dingin(float x){ if (x <= 15) return 1; if (x >= 25) return 0; return (25 - x) / 10.0; }
float mu_normal(float x){ if (x <= 20 || x >= 30) return 0; if (x <= 25) return (x - 20) / 5.0; return (30 - x) / 5.0; }
float mu_panas(float x){ if (x <= 28) return 0; if (x >= 40) return 1; return (x - 28) / 12.0; }

float mu_kering(float x){ if (x <= 20) return 1; if (x >= 60) return 0; return (60 - x) / 40.0; }
float mu_normal_tanah(float x){ if (x <= 40 || x >= 80) return 0; if (x <= 60) return (x - 40) / 20.0; return (80 - x) / 20.0; }
float mu_basah(float x){ if (x <= 70) return 0; if (x >= 100) return 1; return (x - 70) / 30.0; }

float mu_cahaya_rendah(float x){ if (x <= 1000) return 1; if (x >= 6000) return 0; return (6000 - x) / 5000.0; }
float mu_cahaya_normal(float x){ if (x <= 4000 || x >= 12000) return 0; if (x <= 8000) return (x - 4000) / 4000.0; return (12000 - x) / 4000.0; }
float mu_cahaya_tinggi(float x){ if (x <= 9000) return 0; if (x >= 15000) return 1; return (x - 9000) / 6000.0; }

int fuzzyFan(float t){
  float d = mu_dingin(t); float n = mu_normal(t); float p = mu_panas(t);
  float den = d+n+p; return (den == 0) ? 0 : (d*0 + n*50 + p*100) / den;
}
int fuzzyLamp(float lux){
  float r = mu_cahaya_rendah(lux); float n = mu_cahaya_normal(lux); float t = mu_cahaya_tinggi(lux);
  float den = r+n+t; return (den == 0) ? 0 : (r*255 + n*120 + t*0) / den;
}
float fuzzyPump(float s){
  float k = mu_kering(s); float n = mu_normal_tanah(s); float b = mu_basah(s);
  float den = k+n+b; return (den == 0) ? 0 : (k*15 + n*8 + b*0) / den;
}

/* ================= CORE FUNCTIONS ================= */

void updateSystem() {
  // Sensor Suhu & Cahaya
  float sRaw = dht.readTemperature();
  if (!isnan(sRaw)) suhu = sRaw;
  
  float luxRaw = lightMeter.readLightLevel();
  luxFiltered = 0.7 * luxFiltered + 0.3 * luxRaw;

  // Sensor Tanah
  static unsigned long lastSoilRead = 0;
  if (millis() - lastSoilRead >= 900000 || lastSoilRead == 0) {
    int adcSoil = analogRead(SOIL_PIN);
    soil = map(adcSoil, 4095, 2000, 0, 100);
    soil = constrain(soil, 0, 100);
    
    pumpSec = fuzzyPump(soil); 
    if (pumpSec > 0.5) {
      digitalWrite(RELAY_PIN, LOW); 
      timer.setTimeout(pumpSec * 1000, []() {
        digitalWrite(RELAY_PIN, HIGH); 
      });
    }
    lastSoilRead = millis();
    Serial.println(">>> Sensor Tanah & Pompa Diupdate (Interval 1 Menit)");
  }

  fanPercent = fuzzyFan(suhu);
  lampPWM = (luxFiltered > 12000) ? 0 : fuzzyLamp(luxFiltered);
  if (lampPWM < 10) lampPWM = 0;

  ledcWrite(FANPIN, map(fanPercent, 0, 100, 0, 255));
  ledcWrite(LEDPIN, lampPWM);
}

void sendToBlynk() {
  Blynk.virtualWrite(V0, suhu);
  Blynk.virtualWrite(V1, soil);
  Blynk.virtualWrite(V2, luxFiltered);
  Blynk.virtualWrite(V3, fanPercent);
  Blynk.virtualWrite(V4, pumpSec);
  Blynk.virtualWrite(V5, lampPWM);
}

void sendToDatabase() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(scriptURL) + "?temp=" + String(suhu) + "&soil=" + String(soil) +
                 "&light=" + String(luxFiltered) + "&fan=" + String(fanPercent) +
                 "&pump=" + String(pumpSec) + "&lamp=" + String(lampPWM);
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("Database Sent! (Interval 5 Menit) Response: %d\n", httpCode);
    http.end();
  }
}

/* ================= SETUP & LOOP ================= */

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  dht.begin();
  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin();
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  ledcAttach(FANPIN, pwmFreq, pwmRes);
  ledcAttach(LEDPIN, pwmFreq, pwmRes);

  // JADWAL INTERVAL
  timer.setInterval(10000L, updateSystem);     // Kontrol Lokal (10s)
  timer.setInterval(15000L, sendToBlynk);      // Blynk Cloud (15s)
  timer.setInterval(300000L, sendToDatabase);  // Google Sheets (5 Menit)
}

void loop() {
  Blynk.run();
  timer.run();
}
