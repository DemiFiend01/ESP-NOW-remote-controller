#include <esp_now.h>
#include <WiFi.h>

struct message{
  int x,y;
};

message rx_message;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&rx_message, incomingData,len);
  Serial.println(rx_message.x);
  Serial.println(rx_message.y);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial.println("dd");
  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("The receiver has been initiated");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);

}
