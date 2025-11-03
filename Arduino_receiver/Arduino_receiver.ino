#include <esp_now.h>
#include <WiFi.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
} struct_message;

struct_message myData;

const int ledPin = 5;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, len);
  // Turn LED on/off

  Serial.print("X: ");
  Serial.println(myData.x);
  Serial.print("Y: ");
  Serial.println(myData.y);
  Serial.print("Start: ");
  Serial.println(myData.start);
  Serial.print("Select: ");
  Serial.println(myData.select);
  Serial.print("X button: ");
  Serial.println(myData.x_button);
  Serial.print("Y button: ");
  Serial.println(myData.y_button);
  Serial.print("A button: ");
  Serial.println(myData.a_button);
  Serial.print("B button: ");
  Serial.println(myData.b_button);
  // digitalWrite(ledPin, HIGH);
  // delay(500);
  // digitalWrite(ledPin, LOW);
}
 
void setup() {
  //pinMode(ledPin, OUTPUT);

  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("hey");
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully initialized, we will register our callback function to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}
 
void loop() {
  //Serial.println("hey");
  //delay(400);
}

   