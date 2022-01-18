
// Bluetooth Thermostat using the ESP32 WROOM32 and OLED display
// use the app "Serial Bluetooth Terminal" to control the thermostat; send the text 'help' for options
// October 2021, Anne de Jong

#include <OneWire.h>
#include <DallasTemperature.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================================== PARAMETERS ===================================================

int TEMP_THRESHOLD_LOWER = 19; // upper threshold of temperature
int TEMP_THRESHOLD_UPPER = 20; // lower threshold of temperature
int TEMP_SAVE = 3 ;  // Overheat protection; TEMP_THRESHOLD_UPPER + TEMP_SAVE

// UPPER value for the second relay to prevent overheating is relay 1 would fail
String HEATER_STATUS = "OFF";
int LOG_INTERVAL = 6 ;// log sample time in seconds
unsigned long LastLogMillis ; 
unsigned long LastShutdown ;
const int LOG_COUNT_MAX = 20000  ;
float logValues[LOG_COUNT_MAX] ; 
int logCount = 0 ;

// ===================================== PIN CONNECTIONS ==============================================


// PIN CONNECTIONS relay KY-019 of EZ-Delivery
const int RELAY_PIN = 27;   // ESP32 pin connected to relay KY-019 of EZ-Delivery | connectors= +(VCC 3.3 or 5V);-(GND);s(Signal)

// PIN CONNECTIONS; DS18B20 digital thermometer
const int oneWireBus = 14;  // GPIO14 pin is used to read the data from the DS18B20 (yellow wire)    
                            // DS18B20 Board connections: red 3-5V;  black GND;  yellow, GPIO14 and 4.7kOhm to 3-5V (same is 4.7k between red and yellow wire)

// PIN CONNECTIONS; SSD1306 OLED display display connected to I2C (SDA_21, SCL_22 pins); 
// connectors=GND;VCC;SCL;SDA ==> gnd; 3.3V; GPIO22; GPIO21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



// Digital thermometer
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

// WROOM32 Bluetooth
BluetoothSerial SerialBT;



void setup() {
  Serial.begin(9600); // Start the Serial Monitor
  SerialBT.begin("Anne_Thermostat"); //Bluetooth device name
  sensors.begin(); // Start the DS18B20 sensor
  pinMode(RELAY_PIN, OUTPUT);


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 1 second
  display.clearDisplay();

  OLED_Intro() ;
  delay(20000); // Pause for 3 second
  
  LastLogMillis = millis();  //initial start time
  LastShutdown  = millis();  //initial start time
}

void myprint(String str) {
  SerialBT.println(str); 
  Serial.println(str);
}

void help() {
  myprint("[U23 or u23], set UPPER limit to 23°C");
  myprint("[L20 or l20], set LOWER limit to 20°C");
  myprint("[S4 or s4], set SAVE limit to UPPER + 2°C");
  myprint("[I60 or i60 or logI60 or logi60], set log interval to 60 seconds");
  myprint("[log], show all logged values");
  myprint("[logReset or logreset], restart logging data");
  myprint("[limits], show current limits and relay status");
  myprint("[status], show current limits and relay status");
  myprint("[relay], show current limits and relay status");
  myprint("[reset], reset upper and lower limit");
  myprint("[help], print this help");
  myprint("All input is not case sensitive");
  myprint("---- Script written by Anne de Jong ----");
}

void Limits() {
  myprint("Current LOWER limit = "+String(TEMP_THRESHOLD_LOWER)+ "°C");
  myprint("Current UPPER limit = "+String(TEMP_THRESHOLD_UPPER)+ "°C");
  if (HEATER_STATUS == "ON") {
     myprint("The heater is turned ON"); }
  else {myprint("The heater is turned OFF");}   
}

void ResetLimits() {
  TEMP_THRESHOLD_UPPER = 21;
  TEMP_THRESHOLD_LOWER = 19;
  myprint("Reset LOWER limit to "+String(TEMP_THRESHOLD_LOWER)+ "°C");
  myprint("Reset UPPER limit to "+String(TEMP_THRESHOLD_UPPER)+ "°C");
  
}

void SetNewValues(String Query) {
    if(Query.indexOf("U") == 0 ) {   // e.g., u33
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal!=0 && QueryVal>TEMP_THRESHOLD_LOWER) {
          myprint("New UPPER limit = "+String(QueryVal)+ "°C");
          OLED_Message("New UPPER limit = "+String(QueryVal));
  

          TEMP_THRESHOLD_UPPER = QueryVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
          OLED_Message("ERROR: UPPER should be higher than LOWER limit");
        }
    } 
    if(Query.indexOf("L") == 0 ) {   // e.g., L28  
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal!=0 && QueryVal<TEMP_THRESHOLD_UPPER) {
        myprint("New LOWER limit = "+String(QueryVal)+ "°C");
        OLED_Message("New LOWER limit = "+String(QueryVal));
        TEMP_THRESHOLD_LOWER = QueryVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
          OLED_Message("ERROR: UPPER should be higher than LOWER limit");
        } 
     }
     if(Query.indexOf("I") == 0 ) {   // e.g., i60  set log interval  
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal>5 && QueryVal<3601) {
        myprint("New Log Interval = "+String(QueryVal)+ " seconds");
        OLED_Message("New Log Interval = "+String(QueryVal)+ " seconds");
        LOG_INTERVAL = QueryVal;
        } else { 
          myprint("ERROR: Log interval should be between 6 and 3600 seconds") ; 
          OLED_Message("ERROR: Log interval should be between 6 and 3600 seconds") ; 
        } 
     }
    
}


void logTemperature(float temperature) {  // add value to the log array
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {
    myprint("Add to log; "+ String(logCount) + " ; t=" + String(logCount*LOG_INTERVAL) + " ; " + String(temperature) +"°C");
    logValues[logCount] = temperature ;
    logCount += 1 ;
    LastLogMillis = millis();
  }
}

void logShow() {  // show the log
  myprint("ID,Seconds,Minutes,Hours,Temp/°C");
  for (int i=0; i<logCount; i++) {
    myprint(String(i) + "\t" +String(i*LOG_INTERVAL)+ "\t" +String(i*LOG_INTERVAL/60)+ "\t" +String(i*LOG_INTERVAL/3600) + "\t" + String(logValues[i]));
  }  
}



void OLED_plot() {
  // plot the last 100 values on the screen of 128 x 64px
  int xRange = 100 ; // 100 pixels
  int yRange = 50 ; // 50 pixels
  float maxTemp = -273 ;
  float minTemp = +273 ;
  int startCount = 0 ;
  if (logCount > xRange ) { startCount = logCount - xRange; }
  // calculate min, max and mean
  float sum = 0 ; 
  for (int i=startCount; i<logCount; i++) {  
      sum += logValues[i] ;
      maxTemp = max(maxTemp, logValues[i]);  
      minTemp = min(minTemp, logValues[i]);
  }
  float mean = sum / (logCount - startCount) ;
  minTemp -= 1  ;  // Here I use +/- 1 °C for plotting limits
  maxTemp += 1  ;
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(5,4);
  display.print("mean:");
  display.setTextSize(2);
  display.setCursor(35,0);
  display.print(FloatToStr(mean,1)+(char)247 + "C") ;
  display.setTextSize(1);
  display.setCursor(0,19);
  display.print(FloatToStr(maxTemp,1)) ;
  display.setCursor(0,54);
  display.print(FloatToStr(minTemp,1)) ;  
  // Plot scaled values to fit the OLED size
  float scale = yRange / (maxTemp - minTemp) ;
  for (int i=startCount; i<logCount; i++) { 
    int y = int((logValues[i] -minTemp) * scale);  // Add the last values
    display.drawPixel(128-(i-startCount), 64-y, WHITE);
  }
  // draw a dotted mean line
  int meanScale = int((mean -minTemp) * scale); 
  for (int i = 0; i < xRange; i=i+3) {  
    display.drawPixel(128-i, 64-meanScale, WHITE);
  }  
  display.display();
}



String FloatToStr(float f,int d) {
  int decimal = round((f - int(f)) * pow(10,d)) ; 
  return String(int(f)) + "." + String(decimal) ;
}


void OLED_Intro() {
  // Show info on OLED
  // Current temperature
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Bluetooth Thermostat");
  display.setCursor(0,10);
  display.print("Name: Anne_Thermostat");
  display.setCursor(0,20);
  display.print("BT options: type help");
  display.setCursor(0,30);
  display.print("www.hondsrugbier.nl");
  display.setCursor(20,40);
  display.print("Anne de Jong");
  display.setCursor(20,50);
  display.print("10-12-2021");
  display.setCursor(0,60);
  display.print("");
  display.display();
  
}  


void OLED_Message(String msg) {
  // Show Message on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Settings update") ;
  display.setCursor(0,30);
  display.print(msg) ;
  display.display();
  delay(6000) ;
}  
  


void OLED_text(float temperature) {
  // Show info on OLED
  // Current temperature
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(35,0);
  display.print(FloatToStr(temperature,1)+(char)247 + "C") ;
  display.setCursor(0, 20);
  display.println("HEATER:"+HEATER_STATUS);
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print("Lower limit = " + String(TEMP_THRESHOLD_LOWER)+(char)247 + "C");
  display.setCursor(0, 46);
  display.print("Upper Limit = " + String(TEMP_THRESHOLD_UPPER)+(char)247 + "C");
  display.setCursor(0, 56);
  display.print("Log Interval= " + String(LOG_INTERVAL) + " s");
  display.display();
}  



void loop() {
  // read the temperature
  sensors.requestTemperatures(); 
  float temperature = sensors.getTempCByIndex(0);
  myprint(String(temperature)+"°C");

  logTemperature(temperature) ;


  // Tijdelijke oplossing reset heater: 1x per 3600 seconds, 5 seconds off
  if (millis() - LastShutdown > 1000*3600) {
    myprint("RESET: Power OFF for 5 seconds");
    digitalWrite(RELAY_PIN, LOW); // turn OFF
    HEATER_STATUS == "OFF" ;
    LastShutdown = millis();
    delay(5000) ;
  }
  
  // send temperature data via Bluetooth
  if (SerialBT.available()) {  
    // Read new limits from BT; U or u for Upper L or l for Lower; e.g.,  U33 or L21
    String BTquery=SerialBT.readString();
    BTquery.toUpperCase();
    BTquery.trim();
    if (BTquery == "RESET" ) { ResetLimits(); }
    else if (BTquery == "HELP" ) { help(); }
    else if (BTquery == "LOG" ) { logShow(); }
    else if (BTquery == "LOGRESET" ) { logCount=0; }
    else if (BTquery == "LIMITS" | BTquery == "STATUS" | BTquery == "RELAY" ) { Limits(); }
    else { SetNewValues(BTquery); }  // Upper, Lower and log Inteval
  }

  // Switch relay ON or OFF
  if(temperature < TEMP_THRESHOLD_LOWER){
    if (HEATER_STATUS == "OFF") {myprint("The heater is turned ON"); }
    digitalWrite(RELAY_PIN, HIGH); // turn on
    HEATER_STATUS = "ON" ;
  } else if(temperature > TEMP_THRESHOLD_UPPER){
    if (HEATER_STATUS == "ON") {myprint("The heater is turned OFF"); }
    digitalWrite(RELAY_PIN, LOW); // turn OFF
    HEATER_STATUS = "OFF" ;
  } 
  if(temperature > TEMP_THRESHOLD_UPPER + TEMP_SAVE){
    myprint("WARNING, Overheating detected; shutting down relay 2");
  }

  OLED_text(temperature) ;
  delay(5000);
  OLED_plot() ;
  delay(5000);
  
  
}
