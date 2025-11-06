#include <Wire.h>
#include "Adafruit_seesaw.h"
#include <esp_now.h>
#include <WiFi.h>
Adafruit_seesaw ss;

#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xDC, 0x06, 0x75, 0xF7, 0xE8, 0x58};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  Wire.begin(8,9);
  delay(50);

  if(!ss.begin(0x50)){
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  }

  Serial.println("Gamepad QT example!");
  Serial.println("seesaw started");
  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 5743) {
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  
  Serial.println("Found Product 5743");
  
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }


}

int last_x = 0, last_y = 0;
 
void loop() {
  // Set values to send
  int x = 1023 - ss.analogRead(14);
  int y = 1023 - ss.analogRead(15);
  myData.a_button = false;
  myData.b_button = false;
  myData.x_button = false;
  myData.y_button = false;
  myData.select = false;
  myData.start = false;

  if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
    Serial.print("x: "); Serial.print(x); Serial.print(", "); Serial.print("y: "); Serial.println(y);
    last_x = x;
    last_y = y;
    myData.x = x;
    myData.y = y;
  }

  uint32_t buttons = ss.digitalReadBulk(button_mask);

    //Serial.println(buttons, BIN);

  if (! (buttons & (1UL << BUTTON_A))) {
    Serial.println("Button A pressed");
    myData.a_button = true;
  }
  if (! (buttons & (1UL << BUTTON_B))) {
    Serial.println("Button B pressed");
    myData.b_button = true;
  }
  if (! (buttons & (1UL << BUTTON_Y))) {
    Serial.println("Button Y pressed");
    myData.y_button = true;
  }
  if (! (buttons & (1UL << BUTTON_X))) {
    Serial.println("Button X pressed");
    myData.x_button = true;
  }
  if (! (buttons & (1UL << BUTTON_SELECT))) {
    Serial.println("Button SELECT pressed");
    myData.select = true;
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    Serial.println("Button START pressed");
    myData.start = true;
  }

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(10);
}