#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuration constants
#define EEPROM_SIZE 512
#define SSID_LENGTH 32
#define PASSWORD_LENGTH 64
#define MQTT_RECONNECT_DELAY 5000
#define SERIAL_TIMEOUT 500

// Global objects
AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

// Network configuration
struct NetworkConfig {
  String ssid;
  String password;
  String mqttServer;
  String mqttTopic;
  const char* clientId = "ESP01S_Client";
} config;

// EEPROM utility functions
void writeEEPROMString(int start, int maxLength, const String& value) {
  for (int i = 0; i < maxLength; i++) {
    EEPROM.write(start + i, i < value.length() ? value[i] : 0);
  }
  EEPROM.commit();
}

String readEEPROMString(int start, int maxLength) {
  char buffer[maxLength + 1];
  memset(buffer, 0, maxLength + 1);
  for (int i = 0; i < maxLength; i++) {
    buffer[i] = EEPROM.read(start + i);
    if (buffer[i] == 0) break;
  }
  return String(buffer);
}

// WiFi configuration AP
void startConfigAP() {
  const char* apSSID = "ESP-01S-Config";
  const char* apPassword = "123456789";
  WiFi.softAP(apSSID, apPassword);
  Serial.printf("AP Started - SSID: %s, IP: %s\n", apSSID, WiFi.softAPIP().toString().c_str());
}

// Web server handlers
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = R"(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Wi-Fi Setup</title>
        <style>
          body {font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f4f4f4;}
          .container {max-width: 400px; margin: 0 auto; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);}
          h1 {text-align: center; color: #333;}
          label {display: block; margin: 10px 0 5px; color: #666;}
          input[type="text"], input[type="password"] {width: 100%; padding: 8px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box;}
          input[type="submit"] {width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer;}
          input[type="submit"]:hover {background: #45a049;}
        </style>
      </head>
      <body>
        <div class="container">
          <h1>Wi-Fi & MQTT Setup</h1>
          <form action="/save" method="POST">
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid" required maxlength="32" value=")"
                  + config.ssid + R"(">
            <label for="password">Password:</label>
            <input type="password" id="password" name="password" required maxlength="64" value=")"
                  + config.password + R"(">
            <label for="mqttServer">MQTT Server:</label>
            <input type="text" id="mqttServer" name="mqttServer" required maxlength="64" value=")"
                  + config.mqttServer + R"(">
            <label for="mqttTopic">MQTT Topic:</label>
            <input type="text" id="mqttTopic" name="mqttTopic" required maxlength="64" value=")"
                  + config.mqttTopic + R"(">
            <input type="submit" value="Save">
          </form>
        </div>
      </body>
      </html>
    )";
    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true) || !request->hasParam("mqttServer", true) || !request->hasParam("mqttTopic", true)) {
      request->send(400, "text/html", "<h1>Error</h1><p>Missing credentials or MQTT settings</p>");
      return;
    }

    config.ssid = request->getParam("ssid", true)->value();
    config.password = request->getParam("password", true)->value();
    config.mqttServer = request->getParam("mqttServer", true)->value();
    config.mqttTopic = request->getParam("mqttTopic", true)->value();

    if (config.ssid.length() > SSID_LENGTH || config.password.length() > PASSWORD_LENGTH || config.mqttServer.length() > 64 || config.mqttTopic.length() > 64) {
      request->send(400, "text/html", "<h1>Error</h1><p>Input too long</p>");
      return;
    }

    // Write updated values to EEPROM
    writeEEPROMString(0, SSID_LENGTH, config.ssid);
    writeEEPROMString(SSID_LENGTH, PASSWORD_LENGTH, config.password);
    writeEEPROMString(SSID_LENGTH + PASSWORD_LENGTH, 64, config.mqttServer);
    writeEEPROMString(SSID_LENGTH + PASSWORD_LENGTH + 64, 64, config.mqttTopic);

    request->send(200, "text/html", R"(
      <h1>Success</h1>
      <p>Credentials and MQTT settings saved. Rebooting in 2 seconds...</p>
    )");

    delay(2000);
    ESP.restart();
  });

  server.begin();
  Serial.println("Web server started");
}

// Network connection functions
bool connectToWiFi() {
  if (config.ssid.isEmpty() || config.password.isEmpty()) return false;

  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  Serial.print("Connecting to WiFi");

  int attempts = 0;
  const int maxAttempts = 20;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void connectToMQTT() {
  client.setServer(config.mqttServer.c_str(), 1883);
  while (!client.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connecting to MQTT (%s)... ", config.mqttServer.c_str());
    if (client.connect(config.clientId)) {
      Serial.println("connected");
      client.publish("home_esp01s/status", "ESP-01S connected to MQTT");
    } else {
      Serial.printf("failed (rc=%d), retrying in %dms\n", client.state(), MQTT_RECONNECT_DELAY);
      delay(MQTT_RECONNECT_DELAY);
    }
  }
}

// Serial data handling
String readSerialData() {
  String data;
  unsigned long timeout = millis() + SERIAL_TIMEOUT;

  while (millis() < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      data += c;
      if (c == '\n') break;
    }
    yield();  // Prevent watchdog reset
  }
  data.trim();
  return data;
}

void processSerialData() {
  String message = readSerialData();
  if (message.isEmpty()) return;

  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return;
  }

  if (client.publish(config.mqttTopic.c_str(), message.c_str())) {
    Serial.printf("Published to %s: %s\n", config.mqttTopic.c_str(), message.c_str());
    client.publish("home_esp01s/status", "ESP-01S entering deep sleep");
    ESP.deepSleep(0);
  } else {
    Serial.println("MQTT publish failed");
  }
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);

  config.ssid = readEEPROMString(0, SSID_LENGTH);
  config.password = readEEPROMString(SSID_LENGTH, PASSWORD_LENGTH);
  config.mqttServer = readEEPROMString(SSID_LENGTH + PASSWORD_LENGTH, 64);
  config.mqttTopic = readEEPROMString(SSID_LENGTH + PASSWORD_LENGTH + 64, 64);

  if (config.ssid.isEmpty() || config.password.isEmpty()) {
    startConfigAP();
    setupWebServer();
  } else if (connectToWiFi()) {
    Serial.printf("Connected to %s (IP: %s)\n", config.ssid.c_str(), WiFi.localIP().toString().c_str());
    connectToMQTT();
  } else {
    Serial.println("WiFi connection failed, starting config mode");
    startConfigAP();
    setupWebServer();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && !config.ssid.isEmpty()) {
    Serial.println("WiFi disconnected, attempting reconnect...");
    connectToWiFi();
  }

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    connectToMQTT();
  }

  client.loop();

  if (Serial.available()) {
    processSerialData();
  }
}
