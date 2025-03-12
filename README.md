# ESP8266 Wi-Fi and MQTT Controller

This project configures an ESP8266 microcontroller as a Wi-Fi and MQTT controller. It features a web-based configuration portal for Wi-Fi credentials and provides MQTT messaging support.

## Features

- **Wi-Fi Configuration**: A captive portal setup to easily configure network credentials.
- **Persistent Storage**: Saves SSID and password in the EEPROM.
- **MQTT Client**: Publishes and subscribes to MQTT topics for IoT integration.
- **Web Server**: Serves a simple web page for Wi-Fi setup.
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
3. Flash the code to your ESP8266.

### Usage

#### First-Time Configuration

1. Power the ESP8266.
2. Connect to the Wi-Fi network `ESP-01S-Config` with the password `123456789`.
3. Open a web browser and navigate to `192.168.4.1`.
4. Enter your Wi-Fi SSID and password.
5. Save the configuration. The device will reboot and connect to the specified Wi-Fi network.

#### MQTT Integration

- The device automatically connects to the MQTT server at `192.168.0.167`.  
- It subscribes to and publishes messages on the topic `WeatherStation`.

### Customization

- Modify the default AP SSID and password in the `startConfigAP` function.
- Change MQTT server details in the `NetworkConfig` struct.

### Debugging

- Use the Serial Monitor at a baud rate of 9600 to view debug messages.

## License

This project is open-source and can be used or modified freely. Attribution is appreciated.

---

Happy coding!
