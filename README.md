# Sistem Monitoring dan Kontrol Perangkat Berbasis ESP32 Terintegrasi WiFi dan MQTT - Quantum Alert System

## Deskripsi Proyek
Sistem Quantum Alert System merupakan sistem monitoring dan kontrol perangkat berbasis mikrokontroler ESP32 yang terintegrasi jaringan WiFi dan protokol komunikasi MQTT.
Sistem ini dirancang untuk:
-	Memonitor status perangkat secara realtime
-	Mengontrol mode operasi baik secara lokal (tombol) maupun remote (dashboard/web)
-	Memberikan notifikasi visual (LED) dan audio (buzzer)
-	Menyimpan konfigurasi mode dan level pada memori non-volatile
-	Mengirim dan menerima data melalui broker MQTT menggunakan koneksi TLS (secure)
---
## Peran ESP32
ESP32 berperan sebagai:
-	Controller → Mengolah input & logika sistem
-	Publisher → Mengirim status ke broker
-	Subscriber → Menerima perintah dari dashboard
Dengan konsep ini, sistem sudah masuk kategori IoT Embedded Monitoring & Control System.

---

## Hardware yang Digunakan

| Komponen | GPIO ESP32 |
|---------|--------------|
| Button Mode | 18 |
| Button Level | 19 |
| LED Normal | 23 |
| LED Alert | 5 |
| LED WiFi | 16 |
| Buzzer | 4 |

---

## Library yang Digunakan
```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>  
```
---

# Konsep Mikroprosesor yang Diimplementasikan

---

## 1. Koneksi WiFi & MQTT

ESP32 terhubung ke jaringan WiFi dan broker MQTT menggunakan TLS.

```cpp
const char* ssid = "Home-PRD-5G";
const char* password = "******";

const char* mqtt_server =
"2083e7657f56476fb8cd14cff9ad3653.s1.eu.hivemq.cloud";

const int mqtt_port = 8883;
```
---

## 2. Interrupt Handling
Input tombol menggunakan interrupt tanpa polling.

```cpp
attachInterrupt(BTN1, isrBtn1, FALLING);
```
---

## 3. Manajemen Mode Sistem
Pada sistem ini menggunakan 3 mode.

```cpp
String modeName[3] = {
  "NORMAL", "ALERT", "SEQUENCE"
};
```
Perubahan mode dilakukan melalui tombol atau MQTT

## 4. Kontrol Output PWM
Intensitas LED diatur menggunakan PWM berdasarkan level.
```cpp
int getBrightness() {
  switch (level) {
    case 1: return 60;
    case 2: return 120;
    case 3: return 200;
    case 4: return 255;
  }
}
```

Implementasi kontrol LED:
```cpp
void updateLED() {

  int bright = getBrightness();

  ledcWrite(LED_NORMAL,
            mode == 0 ? bright : 0);

  ledcWrite(LED_ALERT,
            mode == 1 ? bright : 0);
}
```
## 5. Mode Sequence LED
Pada mode ini LED berkedip bergantian sebagai indikator alarm.

```cpp
void sequenceLED() {

  int bright = getBrightness();

  ledcWrite(LED_NORMAL, bright);
  ledcWrite(LED_ALERT, 0);
  delay(150);

  ledcWrite(LED_NORMAL, 0);
  ledcWrite(LED_ALERT, bright);
  delay(150);
}
```
Perubahan mode dilakukan melalui tombol atau MQTT

## 6. Kontrol Buzzer
Buzzer aktif hanya pada saat mode ALERT.
```cpp
void updateBuzzer() {

  if (mode != 1) {
    digitalWrite(BUZZER, LOW);
    return;
  }

  digitalWrite(BUZZER, HIGH);
}
```
---
## 7. Penyimpanan Konfigurasi (Preferences)
Data konfigurasi sistem disimpan pada Non-Volatile Storage.
### Load Data
```cpp
void loadPreferences() {
  prefs.begin("alertSys", true);
  mode = prefs.getInt("mode", 0);
  level = prefs.getInt("level", 1);
  prefs.end();
}
```
### Save Data
```cpp
void savePreferences() {
  prefs.begin("alertSys", false);
  prefs.putInt("mode", mode);
  prefs.putInt("level", level);
  prefs.end();
```
Data tetap tersimpan meskipun perangkat dimatikan.

## 8. Publish Status MQTT
ESP32 mengirim data monitoring ke broker.
```cpp
void publishStatus() {

  StaticJsonDocument<200> doc;

  doc["mode"] = modeName[mode];
  doc["level"] = level;
  doc["brightness"] = getBrightness();
  doc["ip"] = WiFi.localIP().toString();

  char buf[200];
  serializeJson(doc, buf);

  client.publish(topic_status, buf);
}
```

## 9. Receive Command MQTT
ESP32 menerima perintah dari dahsboard.
```cpp
void callback(char* topic,
              byte* payload,
              unsigned int length) {

  String msg;

  for (int i = 0; i < length; i++)
    msg += (char)payload[i];

  StaticJsonDocument<200> doc;

  if (deserializeJson(doc, msg)) return;
}
```
Parsing Command
```cpp
if (doc["mode"]) {
  String m = doc["mode"];
  if (m == "NORMAL") mode = 0;
  if (m == "ALERT") mode = 1;
}
```
---
## Lampiran
- <a href="https://github.com/perdiruhiyat/UAS_Sistem-Mikroprosessor_Perdi-Ruhiyat.git">Github</a>
- <a href="https://drive.google.com/file/d/1aV7lxps2KqNDVFhk7-OB8gJ3oZZXPso1/view?usp=sharing">Video Demo</a>
