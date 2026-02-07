#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ===== WIFI =====
const char* ssid = "Munyenyo";
const char* password = "jangkrik";

// ===== MQTT =====
const char* mqtt_server =
  "2083e7657f56476fb8cd14cff9ad3653.s1.eu.hivemq.cloud";

const int mqtt_port = 8883;
const char* mqtt_user = "mqtt-perdi";
const char* mqtt_pass = "@Perdi123";

const char* topic_status = "esp32/alert/status";
const char* topic_control = "esp32/alert/control";

// ===== OBJECT =====
WiFiClientSecure espClient;
PubSubClient client(espClient);
Preferences prefs;

// ===== PIN =====
#define BTN1 18
#define BTN2 19

#define LED_NORMAL 23
#define LED_ALERT 5
#define LED_WIFI 16

#define BUZZER 4

unsigned long lastBtn1 = 0;
unsigned long lastBtn2 = 0;

const int debounceDelay = 250;

// ===== STATE =====
volatile bool btn1Pressed = false;
volatile bool btn2Pressed = false;

int mode = 0;
int level = 1;


String modeName[3] = {
  "NORMAL", "ALERT", "SEQUENCE"
};

// ===== ISR =====
void IRAM_ATTR isrBtn1() {
  btn1Pressed = true;
}
void IRAM_ATTR isrBtn2() {
  btn2Pressed = true;
}

// ===== WIFI =====
void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());
}

// ===== WIFI LED =====
void wifiIndicator() {

  static unsigned long last = 0;
  static bool state = false;

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_WIFI, HIGH);
    return;
  }

  if (millis() - last > 200) {
    last = millis();
    state = !state;
    digitalWrite(LED_WIFI, state);
  }
}

// ===== PREFERENCES =====
void loadPreferences() {
  prefs.begin("alertSys", true);
  mode = prefs.getInt("mode", 0);
  level = prefs.getInt("level", 1);
  prefs.end();

  if (mode > 2) mode = 0;
}

void savePreferences() {
  prefs.begin("alertSys", false);
  prefs.putInt("mode", mode);
  prefs.putInt("level", level);
  prefs.end();
}

// ===== BRIGHTNESS =====
int getBrightness() {
  switch (level) {
    case 1: return 60;
    case 2: return 120;
    case 3: return 200;
    case 4: return 255;
  }
  return 60;
}

// ===== JSON =====
void publishStatus() {

  if (!client.connected()) return;

  StaticJsonDocument<200> doc;
  doc["mode"] = modeName[mode];
  doc["level"] = level;
  doc["brightness"] = getBrightness();
  doc["ip"] = WiFi.localIP().toString();

  char buf[200];
  serializeJson(doc, buf);

  client.publish(topic_status, buf);
  Serial.println(buf);
}

// ===== CALLBACK =====
void callback(char* topic, byte* payload, unsigned int length) {

  String msg;
  for (int i = 0; i < length; i++)
    msg += (char)payload[i];

  StaticJsonDocument<200> doc;
  if (deserializeJson(doc, msg)) return;

  if (doc["mode"]) {
    String m = doc["mode"];
    if (m == "NORMAL") mode = 0;
    if (m == "ALERT") mode = 1;
    if (m == "SEQUENCE") mode = 2;
  }

  if (doc["level"]) {
    level = doc["level"];
    if (level < 1) level = 1;
    if (level > 4) level = 4;
  }

  savePreferences();
  publishStatus();
}

// ===== MQTT =====
void reconnectMQTT() {
  while (!client.connected()) {

    Serial.print("Connecting MQTT Hive...");
    if (client.connect(
          "ESP32Client",
          mqtt_user,
          mqtt_pass)) {
      client.subscribe(topic_control);
      Serial.println("Connected!");
      publishStatus();
    } else delay(2000);
  }
}

// ===== LED =====
void updateLED() {

  int bright = getBrightness();

  ledcWrite(LED_NORMAL,
            mode == 0 ? bright : 0);

  ledcWrite(LED_ALERT,
            mode == 1 ? bright : 0);
}

// ===== SEQUENCE =====
void sequenceLED() {

  int bright = getBrightness();

  ledcWrite(LED_NORMAL, bright);
  ledcWrite(LED_ALERT, 0);
  delay(150);

  ledcWrite(LED_NORMAL, 0);
  ledcWrite(LED_ALERT, bright);
  delay(150);
}

// ===== BUZZER =====
void updateBuzzer() {

  if (mode != 1) {
    digitalWrite(BUZZER, LOW);
    return;
  }

  digitalWrite(BUZZER, HIGH);
}

// ===== SETUP =====
void setup() {

  Serial.begin(115200);

  pinMode(LED_WIFI, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  ledcAttach(LED_NORMAL, 5000, 8);
  ledcAttach(LED_ALERT, 5000, 8);

  attachInterrupt(BTN1, isrBtn1, FALLING);
  attachInterrupt(BTN2, isrBtn2, FALLING);

  loadPreferences();
  connectWiFi();

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ===== LOOP =====
void loop() {

  if (!client.connected())
    reconnectMQTT();

  client.loop();

  if (btn1Pressed) {

    if (millis() - lastBtn1 > debounceDelay) {

      mode++;
      if (mode > 2) mode = 0;

      savePreferences();
      publishStatus();

      lastBtn1 = millis();
    }

    btn1Pressed = false;
  }

  if (btn2Pressed) {

    if (millis() - lastBtn2 > debounceDelay) {

      level++;
      if (level > 4) level = 1;

      savePreferences();
      publishStatus();

      lastBtn2 = millis();
    }

    btn2Pressed = false;
  }

  if (mode == 2) sequenceLED();
  else updateLED();

  updateBuzzer();
  wifiIndicator();
}
