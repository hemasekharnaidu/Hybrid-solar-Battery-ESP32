#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>

// ================= WIFI DETAILS =================
const char* ssid = "iPhone";
const char* password = "hemu123.";
String apiKey = "SJ9QXXKNRMI19WIF";   // NEW WRITE API KEY

// ================= PIN DEFINITIONS =================
#define SOLAR_ADC   34
#define BAT_ADC     32
#define LDR_PIN     35
#define DHT_PIN     4

#define SOLAR_RELAY 27
#define BAT_RELAY   26

#define DHTTYPE DHT11

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHTTYPE);

// ================= CALIBRATION =================
#define SOLAR_MULTIPLIER 7.66
#define BAT_MULTIPLIER   5.84

const float ADC_REF = 3.3;
const int ADC_MAX = 4095;

unsigned long lastSend = 0;
const long sendInterval = 20000;  // 20 seconds

// ================= READ VOLTAGE =================
float readVoltage(int pin, float multiplier) {
  long sum = 0;
  for (int i = 0; i < 40; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  float adc = sum / 40.0;
  float v_adc = adc * (ADC_REF / ADC_MAX);
  return v_adc * multiplier;
}

// ================= WIFI CONNECT =================
void connectWiFi() {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi Connected");
    delay(1500);
  } else {
    Serial.println("\nWiFi Failed!");
  }
}

// ================= SEND USING RAW TCP =================
void sendToThingSpeak(float solarV, float battV, float temp, float hum, int ldr) {

  if (WiFi.status() == WL_CONNECTED) {

    WiFiClient client;

    if (client.connect("api.thingspeak.com", 80)) {

      String postData = "api_key=" + apiKey +
                        "&field1=" + String(solarV,2) +
                        "&field2=" + String(battV,2) +
                        "&field3=" + String(temp,2) +
                        "&field4=" + String(hum,2) +
                        "&field5=" + String(ldr);

      client.println("POST /update HTTP/1.1");
      client.println("Host: api.thingspeak.com");
      client.println("Connection: close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.print(postData);

      Serial.println("Data Sent To ThingSpeak");

      delay(1000);

      while (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }

      client.stop();
    }
    else {
      Serial.println("Connection Failed");
    }
  }
  else {
    Serial.println("WiFi Lost!");
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  analogSetPinAttenuation(SOLAR_ADC, ADC_11db);
  analogSetPinAttenuation(BAT_ADC, ADC_11db);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  pinMode(SOLAR_RELAY, OUTPUT);
  pinMode(BAT_RELAY, OUTPUT);

  digitalWrite(SOLAR_RELAY, HIGH);
  digitalWrite(BAT_RELAY, HIGH);

  lcd.begin();
  lcd.backlight();
  dht.begin();

  connectWiFi();
}

// ================= LOOP =================
void loop() {

  float solarV = readVoltage(SOLAR_ADC, SOLAR_MULTIPLIER);
  float battV  = readVoltage(BAT_ADC, BAT_MULTIPLIER);

  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  int ldrRaw = analogRead(LDR_PIN);
  int ldrPercent = map(ldrRaw, 0, 4095, 0, 100);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Bat:");
  lcd.print(battV,1);
  lcd.print("V");

  lcd.setCursor(0,1);
  lcd.print("Temp:");
  lcd.print(temperature,0);

  Serial.println("====================================");
  Serial.print("Solar: "); Serial.println(solarV);
  Serial.print("Battery: "); Serial.println(battV);
  Serial.print("Temp: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Light: "); Serial.println(ldrPercent);
  Serial.println("====================================");

  if (millis() - lastSend > sendInterval) {
    lastSend = millis();
    sendToThingSpeak(solarV, battV, temperature, humidity, ldrPercent);
  }

  delay(2000);
}
