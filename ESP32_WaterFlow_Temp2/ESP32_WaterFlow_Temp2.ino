/*

1) YF-B7 Water Flow Sensor met Temperatuur Sensor - Messing - G1/2"
  € 7 by https://www.tinytronics.nl/

2) Nieuwe Ultra-Stille 12V Dc Core Solar Hot Koelwater Circulatie Borstelloze Mini Elektrische Dompelpomp Food Grade
€ 16 by https://dutch.alibaba.com/

3) Elektronica van https://www.amazon.nl/
This is also the 5V Power Supply for the ESP32. Input 12V (can be between 5-35V)
Motor speed control L298N
Digital Temp DS18B2
SSD1306 OLED display
ESP32; AZDelivery ESP-32 


OLED  128 x 64 px
http://oleddisplay.squix.ch/#/home




NOTE: NTC Thermistor: Temp ESP32 een ADC heeft van 0 - 4095
   Connect the 50k NTC to GPIO 4 and 3.3V (no direction), connect GPIO4 to GND with 10k resistor
The NTC values were completely wrong, to solve this I boiled water and insert the NTC and DS18B20
let water cool slowly and record the Digital temp and the Voltage on GPIO4
Perform polynomal regression to correlate Volt and Temp
https://stats.blue/Stats_Suite/polynomial_regression_calculator.html
Vout  y=7.3212x3−33.5077x2+85.8347x−19.269

    
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "font.h"

#define LED_BUILTIN 2

// ===================================== PARAMETERS ===================================================

float TEMP_DESIRED = 24.0;  // desired final temperature

// ===================================== PIN CONNECTIONS ==============================================

// PIN CONNECTIONS; DS18B20 digital thermometer
// DS18B20 Board connections: red 3-5V;  black GND;  yellow, GPIO 14 and 4.7kOhm -> 3-5V
// GPIO 27 pin is used to read the data from the DS18B20 (yellow wire)    
const int oneWireBus = 27;  
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

// PIN CONNECTIONS; SSD1306 OLED display display connected to I2C (SDA_21, SCL_22 pins); 
// connectors=GND;VCC;SCL;SDA ==> gnd; 3.3V; GPIO22; GPIO21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// PIN CONNECTIONS; NTC of the YF-B7 Water Flow Sensor; NOTE NTC is inacurate
// red 3.3V, black GND, Yellow GPIO 4 + 10k -> GND
int ThermistorPin = 4 ;
double adcMax = 4095.0; // ADC resolution 12-bit (0-4095)
double Vs = 5;        // supply voltage
const int NTCarraySize = 10 ;
float NTCarray[NTCarraySize] ;      // Always thake the average of 10 measurement 


// PIN CONNECTIONS; YF-B7 Water Flow Sensor
// red 5V, black GND, Yellow GPIO + 10k -> 5V
#define WATERFLOWSENSOR  26
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

// PIN CONNECTIONS; Motor speed control L298N
// Motor need 12V Powersupply; connect 12V and GND to adaptor
// L298N has 6 pins starting at the 5V connection; EMA, IN1, IN2, IN3, IN4, ENA(EnableB)
// Next to 12V connector is MotorA; - and +
int motorA_IN1 = 32; // IN1
int motorA_IN2 = 33; // IN2
int motorA     = 25; // ENA


// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
const int minSpeed = 100;  // increase this if the motor does not start


// PIN CONNECTION; Potentiometer
// + to 3.3V; - to GND; middle to GPIO 34
const int potPin = 34;

// ===================================== SETUP ====================================================
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup()
{
  Serial.begin(9600);
  sensors.begin(); // Start the DS18B20 sensor


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WATERFLOWSENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(WATERFLOWSENSOR), pulseCounter, FALLING);

  // Fill NTCarray
 for (int i=0; i<NTCarraySize; i++) { NTCarray[i] = 20; }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
//  display.display();
  
  display.clearDisplay();

  // MotorA
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorA, OUTPUT);
  ledcSetup(pwmChannel, freq, resolution); // configure LED PWM functionalitites
  ledcAttachPin(motorA, pwmChannel); // attach the channel to the GPIO to be controlled 
  digitalWrite(motorA_IN1, LOW);  // - at IN1
  digitalWrite(motorA_IN2, HIGH); // + at IN2
  ledcWrite(pwmChannel, 255); // Speed up / initialize motorA for 2 seconds
  delay(2000); 
}

// ===================================== functions ====================================================


float Average_NTCtemp(float newTemp) {
  // NTC is highly fluctuating so here we return the average  of "NTCarraySize" measurements 
  float NTCsum = 0 ;
  for (int i=0; i<NTCarraySize-1; i++) { 
    NTCarray[i] = NTCarray[i+1]; 
    NTCsum += NTCarray[i] ;
  }
  NTCarray[NTCarraySize-1] = newTemp ;
  return (NTCsum+newTemp) / NTCarraySize ;
}


String FloatToStr(float f,int d) {
  int decimal = round((f - int(f)) * pow(10,d)) ; 
  return String(int(f)) + "." + String(decimal) ;
}


float MotorSpeed(float t) {
  float Speed=255 ;
  if (t<20) { Speed=255 ; } 
  else if (t<22) { Speed=220 ; } 
  else if (t<24) { Speed=180 ; } 
  else if (t<26) { Speed=140 ; } 
  else { Speed=100 ; } 
 // Serial.println("Speed: " + String(Speed));
  ledcWrite(pwmChannel, Speed); 
  return Speed ; 
}

float Update_Temp_Desired() {
  float potValue = analogRead(potPin);
  // Value is between 0 and 4095, here we normalize to 20; temp will be between 15 - 35
  float result = int(150+potValue*200/4095)  ; // round 1 decimal
  return result/10;
}

// ===================================== LOOP ====================================================


void loop()
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    // Digital temperature DS18B20
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    // NTC analog (inacurate) temperature
    double NTC = analogRead(ThermistorPin) ;
    double Vout = NTC * Vs/adcMax;
    double Tanne = -5E-06*pow(NTC,2) + 0.0519*NTC - 36; // from Excel Polynomal ^2; y = -5E-06x2 + 0.0519x - 35.693
    
    Tanne =  Average_NTCtemp(Tanne) ;

   
    // Water flow meter
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

     // Set motor speed
    float Speed = MotorSpeed(temperature) ; 
    
    // Report
    Serial.println("\t" + String(temperature)+ "\t" + String(NTC) + "\t" +String(Vout)+  "\t" +String(Tanne) +  "\t" +String(Speed) );

    
    float OLD_TEMP_DESIRED = TEMP_DESIRED ;
    TEMP_DESIRED = Update_Temp_Desired();
    if (abs(TEMP_DESIRED - OLD_TEMP_DESIRED) > 0.3) {  // only show if delta temp >0.3°C
      Serial.println("TEMP_DESIRED = " + String(TEMP_DESIRED));
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(0,0);  display.print("TARGET TEMP") ;
      display.setCursor(20,40); display.print(FloatToStr(TEMP_DESIRED,1) + (char)247 + "C") ;
      display.display();
      delay(500) ;
    } else {
  
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(1);  
      display.setCursor(0,2);   display.print("Out:") ;
      display.setCursor(0,19);  display.print("In:") ;
      display.setTextSize(2);
      display.setCursor(30,0);  display.print(FloatToStr(temperature,1) + (char)247 + "C") ;
      display.setCursor(30,17); display.print(FloatToStr(Tanne,1) + (char)247 + "C");
      display.setCursor(0, 34); display.print(FloatToStr(flowRate,1) + " L/min");
      display.setCursor(0, 51); display.print(String(totalMilliLitres) + " ml");
      display.display();
    }  

    
  }
}
