#include <Wire.h>
#include "Adafruit_seesaw.h"
Adafruit_seesaw ss;

#include <esp_now.h>
#include <WiFi.h>

#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

uint8_t receiverAddress[] = {0xB8, 0xF8, 0x62, 0x2D, 0x58, 0x30};

esp_now_peer_info_t peerInfo;

typedef struct message{
  int x,y;
};

message tx_message;

void constructMessage(message& new_message)
{
  int x = 1023 - ss.analogRead(14);
  int y = 1023 - ss.analogRead(14);
  Serial.print(x); Serial.print(", "); Serial.println(y);
  new_message.x = x;
  new_message.y = y;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
}





void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Wire.begin(8,9);
  delay(50);

  if(!ss.begin(0x50)){
    Serial.println("Error seesaw not found");
    while(1) delay(1);
  }

  Serial.println("Gamepad QT example");

  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 5743) {
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  
  Serial.println("Found Product 5743");
  
  //set up the mask for the i2c minicontroller
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  WiFi.mode(WIFI_STA);
  if(esp_now_init()!= ESP_OK){
    Serial.println("error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  memcpy(peerInfo.peer_addr, receiverAddress,6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo)!=ESP_OK){
    Serial.println("failed to add peer");
    return;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  constructMessage(tx_message);
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &tx_message, sizeof(tx_message));

  if(result == ESP_OK){
    Serial.println("Sent with success");
  }else{
    Serial.println("Error sending the data");
  }

  delay(100);
}
