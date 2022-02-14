// Anne de Jong, Feb 2022, ESP-NOW PIR buzzer alarm: RECEIVER 


#include <esp_now.h>
#include <WiFi.h>

// Upload to ESP32-WROOM-32 device 1 with MAC: 94:B9:7E:E5:1D:60

//uint8_t broadcastAddress[] = {0x94, 0xB9,0x7E,0xE5,0x1F,0x4C};

// ===================================== PIN CONNECTIONS ==============================================

const int led = 25;

// Buzzer settings
const byte buzzerPin = 33;

// ===================================== PARAMETERS ==============================================


int freq = 500;
int channel = 0;
int resolution = 8;
int dutyCycle = 128;

// ===================================== FUNCTIONS ==============================================
 
void onReceiveData(const uint8_t *mac, const uint8_t *data, int len) {
  Serial.print("Received from MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5)Serial.print(":");
  }
 
  int * messagePointer = (int*)data;
  Serial.println();
  Serial.println(*messagePointer);
  digitalWrite(led, HIGH);
  ledcWrite(channel, dutyCycle); // Buzzer ON
  delay(2000);
  digitalWrite(led, LOW);
  ledcWrite(channel, 0); // Buzzer OFF
  
 
}
 
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
 
  if (esp_now_init() != ESP_OK) {  Serial.println("Error initializing ESP-NOW");  return;  }
  esp_now_register_recv_cb(onReceiveData);

  // setup led
  pinMode(led, OUTPUT);

  // Buzzer setup
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(buzzerPin, channel);

    
}
 
void loop() {}
