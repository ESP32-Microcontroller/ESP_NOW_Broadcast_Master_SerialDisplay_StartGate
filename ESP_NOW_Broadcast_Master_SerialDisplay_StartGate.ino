/*
    ESP-NOW Broadcast Master
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to broadcast messages to all devices within the ESP-NOW network.
    This example is intended to be used with the ESP-NOW Broadcast Slave example.

    The master device will broadcast a message every 5 seconds to all devices within the network.
    This will be done using by registering a peer object with the broadcast address.

    The slave devices will receive the broadcasted messages and print them to the Serial Monitor.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <SPI.h>
#include <Wire.h>

#include <ezButton.h>

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6

#define SCL0_Pin 19
#define SDA0_Pin 20

int readyLED = 27;

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.
class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  // Constructor of the class using the broadcast address
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  // Function to properly initialize the ESP-NOW and register the broadcast peer
  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to send a message to all devices within the network
  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      return false;
    }
    char *buffer = (char*)data;
    // Serial.println(buffer);
    digitalWrite(2, HIGH);
    delay(10);
    digitalWrite(2, LOW);
    return true;
  }
};

/* Global Variables */

uint32_t msg_count = 0;
ezButton gateSwitch(26);

// Create a broadcast peer object
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

String getDefaultMacAddress() {

  String mac = "";

  unsigned char mac_base[6] = {0};

  if (esp_efuse_mac_get_default(mac_base) == ESP_OK) {
    char buffer[18];  // 6*2 characters for hex + 5 characters for colons + 1 character for null terminator
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    mac = buffer;
  }

  return mac;
}

int firstFingerButtonPin = 16;
int firstFingerButtonState = LOW;

int secondFingerButtonPin = 17;
int secondFingerButtonState = LOW;

int thirdFingerButtonPin = 21;
int thirdFingerButtonState = LOW;

/* Main */
void setup() {
  pinMode(2, OUTPUT);
  pinMode(firstFingerButtonPin, INPUT); 
  pinMode(secondFingerButtonPin, INPUT); 
  pinMode(thirdFingerButtonPin, INPUT); 
  gateSwitch.setDebounceTime(50);

  Serial.begin(115200);
  Wire.begin(SDA0_Pin, SCL0_Pin);
  Serial.println(F("SSD1306 allocation OK"));
  while (!Serial) {
    delay(10);
  }

  char buffer[256];
  String text = getDefaultMacAddress();
  text.toCharArray(buffer, text.length()+1);
  Serial.println(buffer);

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("Transmitter Broadcast");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Register the broadcast peer
  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize broadcast peer");
    Serial.println("Reebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Setup complete. Listening for events to transmit.");
  pinMode(readyLED, OUTPUT);
  digitalWrite(readyLED, HIGH);
}

void loop() {

  gateSwitch.loop();

  if (gateSwitch.isPressed()) { // START_GATE_OPENED
    char data[32];
    snprintf(data, sizeof(data), "START_GATE_OPENED");

    // snprintf(data, sizeof(data), "#%lu", msg_count++);
    Serial.printf("Broadcasting message: %s\n", data);

    if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) {
      Serial.println("Failed to broadcast message");
    }
    digitalWrite(readyLED, LOW);
  }

  if (gateSwitch.isReleased()) { // START_GATE_CLOSED
    char data[32];
    snprintf(data, sizeof(data), "START_GATE_CLOSED");

    // snprintf(data, sizeof(data), "#%lu", msg_count++);
    Serial.printf("Broadcasting message: %s\n", data);

    if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) {
      Serial.println("Failed to broadcast message");
    }
    digitalWrite(readyLED, HIGH);
  }

  // delay(10);

}
