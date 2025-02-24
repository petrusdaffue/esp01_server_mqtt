#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define RX_PIN 2  // GPIO2 for SoftwareSerial RX
#define TX_PIN 0  // GPIO0 for SoftwareSerial TX

SoftwareSerial arduinoSerial(RX_PIN, TX_PIN);  // RX, TX

#define EEPROM_SIZE 512
#define SSID_LENGTH 32
#define PASSWORD_LENGTH 64

AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqttServer = "192.168.0.167";
const char* mqttTopic = "WeatherStation";

String ssid;
String password;

// Function to read a string from EEPROM
String readEEPROMString(int start, int maxLength) {
  char buffer[maxLength + 1];
  for (int i = 0; i < maxLength; i++) {
    buffer[i] = EEPROM.read(start + i);
    if (buffer[i] == 0) break;  // Stop at null terminator
  }
  buffer[maxLength] = '\0';  // Ensure null termination
  return String(buffer);
}

void setup() {
  Serial.begin(115200);       // ESP-01S uses 115200 by default
  arduinoSerial.begin(9600);  // Adjust baud rate as needed

  EEPROM.begin(EEPROM_SIZE);
  ssid = readEEPROMString(0, SSID_LENGTH);
  password = readEEPROMString(SSID_LENGTH, PASSWORD_LENGTH);

  if (ssid == "" || password == "") {
    Serial.println("No Wi-Fi credentials found, awaiting configuration...");
  }

  WiFi.softAP("ESP-01S-Config", "123456789");
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<!DOCTYPE html>"
                  "<html lang=\"en\">"
                  "<head>"
                  "<meta charset=\"UTF-8\">"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                  "<title>Wi-Fi Setup</title>"
                  "<style>"
                  "body { font-family: Arial, sans-serif; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; background-color: #f4f4f4; }"
                  ".container { width: 90%; max-width: 400px; padding: 20px; background-color: white; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }"
                  "h1 { text-align: center; font-size: 2em; margin-bottom: 20px; }"
                  "label { font-size: 1.1em; margin-bottom: 8px; display: block; }"
                  "input[type='text'], input[type='password'] { width: 95%; padding: 10px; margin-bottom: 15px; border-radius: 4px; border: 1px solid #ccc; }"
                  "input[type='submit'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 1.2em; }"
                  "input[type='submit']:hover { background-color: #45a049; }"
                  "</style>"
                  "</head>"
                  "<body>"
                  "<div class=\"container\">"
                  "<h1>Wi-Fi Setup</h1>"
                  "<form action=\"/save\" method=\"POST\">"
                  "<label for=\"ssid\">SSID:</label>"
                  "<input type=\"text\" id=\"ssid\" name=\"ssid\">"
                  "<label for=\"password\">Password:</label>"
                  "<input type=\"password\" id=\"password\" name=\"password\">"
                  "<input type=\"submit\" value=\"Save\">"
                  "</form>"
                  "</div>"
                  "</body>"
                  "</html>";

    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      ssid = request->getParam("ssid", true)->value();
      password = request->getParam("password", true)->value();

      // Write SSID to EEPROM
      for (int i = 0; i < SSID_LENGTH; i++) {
        if (i < ssid.length()) {
          EEPROM.write(i, ssid[i]);
        } else {
          EEPROM.write(i, 0);  // Null padding
        }
      }

      // Write password to EEPROM
      for (int i = 0; i < PASSWORD_LENGTH; i++) {
        if (i < password.length()) {
          EEPROM.write(SSID_LENGTH + i, password[i]);
        } else {
          EEPROM.write(SSID_LENGTH + i, 0);  // Null padding
        }
      }
      EEPROM.commit();

      // Send a more detailed response
      String response = "<!DOCTYPE html>"
                        "<html>"
                        "<head><meta charset=\"UTF-8\"><title>Setup Complete</title></head>"
                        "<body>"
                        "<h1>Credentials Saved</h1>"
                        "<p>The device is rebooting now. Please wait 5-10 seconds, then connect to your Wi-Fi network.</p>"
                        "</body>"
                        "</html>";
      request->send(200, "text/html", response);

      // Give the client time to receive the response before restarting
      delay(2000);  // Increased from 1000 to 2000 ms
      ESP.restart();
    } else {
      // Handle case where parameters are missing
      request->send(400, "text/html", "<h1>Error</h1><p>Missing SSID or password.</p>");
    }
  });

  server.begin();

  if (ssid != "" && password != "") {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("Connected!");
  }

  client.setServer(mqttServer, 1883);
  connectToMQTT();
}

void connectToMQTT() {
  while (!client.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP01S_Client")) {
      Serial.println("Connected!");
      client.publish("home_esp01s/status", "ESP-01S connected to MQTT!");
    } else {
      Serial.println("Failed, retrying in 5 sec...");
      delay(5000);
    }
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && ssid != "" && password != "") {
    Serial.println("Wi-Fi Disconnected! Reconnecting...");
    WiFi.begin(ssid.c_str(), password.c_str());
    delay(1000);
  }

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    connectToMQTT();
  }

  if (arduinoSerial.available()) {
    String message = arduinoSerial.readStringUntil('\n');  // Read JSON data

    Serial.println(message);

    // Parse JSON data
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return;
    }

    // Extract the data
    float temperature = doc["temperature"];
    float humidity = doc["humidity"];

    // Prepare the MQTT message
    String mqttMessage = message;

    // Send the data to the MQTT broker
    client.publish(mqttTopic, mqttMessage.c_str());
    Serial.println("Data sent to MQTT: " + mqttMessage);
  }

  delay(2000);
  client.loop();
}