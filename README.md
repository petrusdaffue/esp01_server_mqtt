# ESP8266 Wi-Fi and MQTT Controller

This project configures an ESP8266 microcontroller as a Wi-Fi and MQTT controller. It features a web-based configuration portal for Wi-Fi credentials and MQTT server settings. The device can publish and subscribe to MQTT topics for IoT integration.

## Features

- **Wi-Fi Configuration**: A captive portal setup to easily configure network credentials (SSID and password).
- **Persistent Storage**: Saves SSID, password, MQTT server, and topic in the EEPROM for persistence across reboots.
- **MQTT Client**: Publishes and subscribes to MQTT topics for IoT integration.
- **Web Server**: A simple web page serves as the interface for configuring Wi-Fi and MQTT settings.
- **Serial Data Handling**: Processes incoming serial data for further processing and MQTT publishing.

## Getting Started

### Prerequisites

- ESP8266 development board (e.g., ESP-01S)
- Arduino IDE with the following libraries installed:
  - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
  - [PubSubClient](https://github.com/knolleary/pubsubclient)
  - [ArduinoJson](https://arduinojson.org/)
  - [ESP8266WiFi](https://github.com/esp8266/Arduino)

### Installation

1. Install the required libraries in the Arduino IDE.
2. Clone this repository or copy the code to your Arduino IDE.
3. Flash the code to your ESP8266 using the Arduino IDE.

### Usage

#### First-Time Configuration

1. Power the ESP8266.
2. Connect to the Wi-Fi network `ESP-01S-Config` with the password `123456789`.
3. Open a web browser and navigate to `192.168.4.1`.
4. Enter your Wi-Fi SSID, password, MQTT server, and topic information.
5. Click "Save". The device will reboot and automatically connect to the specified Wi-Fi network and MQTT server.

#### MQTT Integration

- The device automatically connects to the MQTT server at `192.168.0.167` (or the configured server if changed).
- It subscribes to and publishes messages on the topic `WeatherStation` (or the configured topic if changed).
- The device will publish MQTT messages to indicate its status, including when it enters deep sleep mode.

### Web-Based Configuration

The web-based configuration allows you to set up Wi-Fi credentials and MQTT settings via a simple HTML form:

1. **Wi-Fi SSID**: The name of your Wi-Fi network.
2. **Wi-Fi Password**: The password for your Wi-Fi network.
3. **MQTT Server**: The IP address or domain name of your MQTT broker.
4. **MQTT Topic**: The topic used to publish or subscribe to messages on the MQTT broker.

### Customization

- Modify the default AP SSID and password in the `startConfigAP` function.
- Change MQTT server details (IP address and topic) in the `NetworkConfig` struct.

### Debugging

- Use the Serial Monitor at a baud rate of 9600 to view debug messages and troubleshooting information.
- If the Wi-Fi credentials or MQTT server settings are incorrect or missing, the device will automatically enter configuration mode (acting as an access point) to allow you to reconfigure the settings.

## License

This project is open-source and can be used or modified freely. Attribution is appreciated.

---

Happy coding!
