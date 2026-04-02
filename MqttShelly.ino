
/*
  ============================================================
  ESP32 – HiveMQ Cloud MQTT
  ============================================================
  Topics:
    SUBSCRIBE : esp32/befehl          ← "ON" / "OFF" von Lazarus
    PUBLISH   : esp32/sensor/status   → "online"
    PUBLISH   : esp32/sensor/led      → "ON" / "OFF" (aktueller Status)

  LED an GPIO 32 (wie bisher)
  ============================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ── Pins ─────────────────────────────────────────────────────
const int LED_PIN = 32;

// ── WiFi ─────────────────────────────────────────────────────
const char* ssid     = "ZTE_B983F8";
const char* password = "99607046";

// ── HiveMQ Cloud ─────────────────────────────────────────────
const char* mqtt_server = "85484da2efb94762bdf2d25a3c8df9a0.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "lazarususer";
const char* mqtt_pass   = "Laz12345";

// ── Topics ───────────────────────────────────────────────────
#define TOPIC_BEFEHL  "esp32/befehl"           // empfangen von Lazarus
#define TOPIC_STATUS  "esp32/sensor/status"    // ESP32 online/offline
#define TOPIC_LED     "esp32/sensor/led"       // aktueller LED-Status

// ── Globale Variablen ─────────────────────────────────────────
WiFiClientSecure espClient;
PubSubClient     client(espClient);
unsigned long    lastMsg  = 0;
bool             ledState = false;

// ── MQTT Callback – wird aufgerufen bei eingehender Nachricht ─
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Payload in String umwandeln
  String msg = "";
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  Serial.print("[MQTT] Topic: ");
  Serial.print(topic);
  Serial.print(" | Payload: ");
  Serial.println(msg);

  // ── esp32/befehl auswerten ──────────────────────────────────
  if (String(topic) == TOPIC_BEFEHL) {
    if (msg == "ON") {
      digitalWrite(LED_PIN, HIGH);
      ledState = true;
      client.publish(TOPIC_LED, "ON");
      Serial.println("  → LED EIN");
    }
    else if (msg == "OFF") {
      digitalWrite(LED_PIN, LOW);
      ledState = false;
      client.publish(TOPIC_LED, "OFF");
      Serial.println("  → LED AUS");
    }
  }
}

// ── WiFi verbinden ────────────────────────────────────────────
void setup_wifi() {
  Serial.print("Verbinde mit: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden – IP: " + WiFi.localIP().toString());
}

// ── MQTT verbinden + abonnieren ───────────────────────────────
void reconnect() {
  while (!client.connected()) {
    Serial.print("Verbinde zu HiveMQ...");
    if (client.connect("ESP32_SmartHome", mqtt_user, mqtt_pass)) {
      Serial.println(" verbunden!");

      // Topic abonnieren
      client.subscribe(TOPIC_BEFEHL);
      Serial.println("Abonniert: " + String(TOPIC_BEFEHL));

      // Online-Status senden
      client.publish(TOPIC_STATUS, "online");
    }
    else {
      Serial.print(" Fehler, rc=");
      Serial.println(client.state());
      Serial.println("Neuer Versuch in 5s...");
      delay(5000);
    }
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setup_wifi();

  espClient.setInsecure();  // TLS ohne Zertifikatsprüfung (wie Shelly)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);  // ← Callback registrieren!
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
  // Verbindung sicherstellen
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // eingehende Nachrichten verarbeiten

  // Alle 10 Sekunden Status senden
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    client.publish(TOPIC_STATUS, "online");
    client.publish(TOPIC_LED, ledState ? "ON" : "OFF");
    Serial.println("Status gesendet – LED: " + String(ledState ? "ON" : "OFF"));
  }
}

