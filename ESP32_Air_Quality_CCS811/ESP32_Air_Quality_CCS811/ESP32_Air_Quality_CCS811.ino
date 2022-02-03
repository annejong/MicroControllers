/*
  ccs811basic.ino - Demo sketch printing results of the CCS811 digital gas sensor for monitoring indoor air quality from ams.
  Created by Maarten Pennings 2017 Dec 11
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>    // I2C library
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ccs811.h"  // CCS811 library
#include <Adafruit_SSD1306.h>  // OLED SSD1306

// ===================================== PIN CONNECTIONS ==============================================

// PIN CONNECTIONS; CCS811 air sensor with nWAKE to 23 (or GND)
// CCS811; VCC,  GND, SCL,    SDA,    WAK,    INT, RST, ADD
// ESP32 ; 3.3V, GND, GPIO22, GPIO21, GPIO23,  -    -    -
CCS811 ccs811(23); // nWAKE on 23


// PIN CONNECTIONS; SSD1306 OLED display 
// SSD1306; GND,  VCC,   SCL,    SDA
// ESP32  ; GND,  3.3V,  GPIO22, GPIO21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// PIN CONNECTIONS; DS18B20 temperature sensor
// DS18B20; GND, Data,                      VCC    (face flat site)
// ESP32;   GND, GPIO14 + 4.7kOhm to 3.3V,  3.3V 
const int oneWireBus = 14;  
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 


// ===================================== PARAMETERS ==============================================

// SSID & Password
const char* ssid = "Hartelijk";  // Enter your SSID here
const char* password = "Welkomja123";  //Enter your Password here
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)
String WiFilocalIP = "not connected";

float temperature = 20 ;

int LOG_INTERVAL = 6 ;// log sample time in seconds
unsigned long LastLogMillis ; 
const int maxX=1000 ;         // number of measurements in graph
uint16_t CO2[maxX] ;             // CO2 values 
uint16_t VOC[maxX] ;             // VOC values 
float TMP[maxX] ;               // TEMPerature values 
const int AVG_SIZE = 50 ;           // To take average of CO2 or VOC values    
uint16_t AVG_eco2[AVG_SIZE] ;
uint16_t AVG_etvoc[AVG_SIZE] ;
String CO2_plotvalues ;            // Hold values as a string for plotting
String VOC_plotvalues ;            // Hold values as a string for plotting
String TMP_plotvalues ;            // Hold values as a string for plotting
String HTML = "";
String X_plotvalues ;
String IAQ_names[5]  = {"excellent","good","moderate","poor","unhealty"}  ; // Standards for Indoor Air Quality (IAQ)
String IAQ_colors[5] = {"blue","green","yellow","orange","red"}  ; // Standards for Indoor Air Quality (IAQ)
uint16_t CO2_IAQ[4] = {400,1000,2000,5000} ;   // excellent < 400 ppm, good< 1000, etc
uint16_t VOC_IAQ[4] = {65, 220,660, 2200};     // excellent < 65 ppb, good <220,  etc

// ===================================== FUNCTIONS ==============================================

void setup() {
  // Enable serial
  Serial.begin(9600);

  // Wifi connection
  Serial.println("Try Connecting to "+ String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(1000);  Serial.print(".");  }
  Serial.println("WiFi connected successfully");
  WiFilocalIP = WiFi.localIP().toString() ;
  Serial.println("Got IP: "+ WiFilocalIP);

  // Start Web server
  server.on("/", handle_root);
  server.begin();
  Serial.println("Web server started");
  delay(100); 

  // Start the DS18B20 digital temp sensor
  Serial.println("Start DS18B20 digital temp sensor");
  sensors.begin(); 

  // Enable I2C
  Wire.begin(); 

  // Start CCS811 sensor
  Serial.println("Starting CCS811");
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok= ccs811.begin();
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Print CCS811 versions
  Serial.print("ccs811 hardware    version: "); Serial.println(ccs811.hardware_version(),HEX);
  Serial.print("ccs811 bootloader  version: "); Serial.println(ccs811.bootloader_version(),HEX);
  Serial.print("ccs811 application version: "); Serial.println(ccs811.application_version(),HEX);
  
  // Start measuring
  ok= ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,0); display.print("ccs811") ;
  display.setTextSize(1);  
  display.setCursor(0,20); display.print("ccs811 versions") ;
  display.setCursor(0,30); display.print("hardware    : " + String(ccs811.hardware_version(),HEX)); 
  display.setCursor(0,40); display.print("bootloader  : " + String(ccs811.bootloader_version(),HEX)); 
  display.setCursor(0,50); display.print("application : " + String(ccs811.application_version(),HEX)); 
  display.display();
  delay(5000); 

  // Setting Plotvalues
  LastLogMillis = millis();  //initial start time
  // Make xValues string
  X_plotvalues = '[' ;
  for (int i = 0; i < maxX; i++) {  X_plotvalues += String(i)+","; }  
  X_plotvalues += String(maxX) + ']' ; // add the last without comma
  // set all initial CO2=400 and VOC=0
  for (int i = 0; i <= maxX; i++) {  CO2[i] = 400; VOC[i] = 0; TMP[i] = 20; }  
  for (int i = 0; i < AVG_SIZE; i++) {  AVG_eco2[i] = 400; AVG_etvoc[i] = 0; }  
  
}

// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}



void UpdateHTML() {
  // HTML & CSS contents which display on web server
  HTML = "<html>" ;
  HTML += "<script src=https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.7.0/chart.min.js></script>";
  HTML += "<body><head><meta http-equiv=refresh content=15></head><h1>Hondsrugbier CO2 and VOC (Volatile Organic Compounds) </h1>" ;
  HTML += "<canvas id=myChart style=width:100%;max-width:1000px></canvas>" ;
  HTML +="<a href=data:application/octet-stream,"+CO2_plotvalues+" download='CO2data.csv'>Download CO2 data</a><br>";
  HTML +="<a href=data:application/octet-stream,"+VOC_plotvalues+" download='VOCdata.csv'>Download VOC data</a><br>";
  HTML +="<a href=data:application/octet-stream,"+TMP_plotvalues+" download='TMPdata.csv'>Download TMP data</a>";
  // Add the chart.js script
  HTML += " \
  <script>                                                     \
    var xValues = "+X_plotvalues+";      \
    var CO2values = "+CO2_plotvalues+";                  \
    var VOCvalues = "+VOC_plotvalues+";                  \
    var TMPvalues = "+TMP_plotvalues+";                  \
    new Chart('myChart', {                                                                                        \
      type:'line',                                                                                                \
      data: {                                                                                                     \
        labels: xValues,                                                                                          \
        datasets: [{                                                                                              \
          label: 'CO2',                                                                                           \
      yAxisID: 'CO2',                                                                                         \
          fill: false,                                                                                            \
          lineTension: 0.3,                                                                                       \
          backgroundColor:'rgba(85,156,44,1.0)',                                                                    \
          borderColor:'rgba(0,0,255,0.1)',                                                                        \
          data: CO2values                                                                                         \
        }, {                                                                                                      \
          label: 'VOC',                                                                                           \
      yAxisID: 'VOC',                                                                                         \
          fill: false,                                                                                            \
          lineTension: 0.3,                                                                                       \
          backgroundColor:'rgba(26,122,186,1.0)',                                                                    \
          borderColor:'rgba(0,0,255,0.1)',                                                                        \
          data: VOCvalues                                                                                         \
        },{                                                                                                      \
          label: 'TMP',                                                                                           \
      yAxisID: 'TMP',                                                                                         \
          fill: false,                                                                                            \
          lineTension: 0.3,                                                                                       \
          backgroundColor:'rgba(245,95,49,1.0)',                                                                    \
          borderColor:'rgba(0,0,255,0.1)',                                                                        \
          data: TMPvalues                                                                                         \
        } ]                                                                                                        \
      },                                                                                                          \
      options: {                                                                                                  \
        legend: {display: false},                                                                                 \
        scales: {                                                                                                 \
      x: {                                                                                                  \
        display: true,                                                                                    \
        title: { display: true, text: 'Time in minutes'}                                          \
      },                                                                                                    \
      CO2: {                                                                                                \
        type: 'linear',                                                                                   \
        display: true,                                                                                    \
        position: 'left',                                                                                 \
        title: { display:true, text: 'ppm CO2' }                                                          \
      },                                                                                                    \
      VOC: {                                                                                                \
        type: 'linear',                                                                                   \
        display: true,                                                                                    \
        position: 'right',                                                                                \
        title: { display:true, text: 'ppb VOC (Volatile Organic Compounds)' },                            \
        grid: { drawOnChartArea: false }                                                                  \
      },                                                                                                    \
      TMP: {                                                                                                \
        type: 'linear',                                                                                   \
        display: true,                                                                                    \
        position: 'right', suggestedMin: 10, suggestedMax: 40,                                                                                \
        title: { display:true, text: 'Temperature in C' },                            \
        grid: { drawOnChartArea: false }                                                                  \
      }                                                                                                     \
        }                                                                                                         \
      }                                                                                                           \
    });                                                                                                           \
  </script>                                                                                                       \
  ";
    
  HTML += "</body></html>";
}


String ArrayToString(uint16_t arr[]) {
  String Str = "[";
  for (int i = 0; i < maxX-1; i++) {    Str += String(arr[i]) + "," ;   }
  return Str += "]";
}

String ArrayTempToString(float arr[]) {
  String Str = "[";
  for (int i = 0; i < maxX-1; i++) {    Str += FloatToStr(arr[i],1) + "," ;   }
  return Str += "]";
}


void add_CO2(uint16_t t) {
  // Shift array and Add CO2 value 
  for (int i = 0; i < maxX-1; i++) {    CO2[i] = CO2[i+1] ;   }
  CO2[maxX-1] = t ;
}

void add_VOC(uint16_t t) {
  // Shift array and Add VOC value 
  for (int i = 0; i < maxX-1; i++) {    VOC[i] = VOC[i+1] ;   }
  VOC[maxX-1] = t ;
}


void add_TMP(float t) {
  // Shift array and Add VOC value 
  for (int i = 0; i < maxX-1; i++) {    TMP[i] = TMP[i+1] ;   }
  TMP[maxX-1] = t ;
}


void UpdateCO2(uint16_t t) {
  // Shift array and Add value 
  for (int i = 0; i < AVG_SIZE-1; i++) { AVG_eco2[i] = AVG_eco2[i+1] ;   }
  AVG_eco2[AVG_SIZE-1] = t ;
}

void UpdateVOC(uint16_t t) {
  // Shift array and Add value 
  for (int i = 0; i < AVG_SIZE-1; i++) { AVG_etvoc[i] = AVG_etvoc[i+1] ;   }
  AVG_etvoc[AVG_SIZE-1] = t ;
}

uint16_t ArrayAverage(uint16_t arr[], int N) {
  float sum=0; for (int i=0;i<N;i++) {sum+=arr[i];}
  return round(sum/N) ;
}


void UpdateCCS811values(uint16_t eco2, uint16_t etvoc) {
  UpdateCO2(eco2) ;   // log last N=AVG_SIZE values
  UpdateVOC(etvoc) ;  // log last N=AVG_SIZE values
  Serial.print("CO2 " + String(eco2)+ " ppm   "); 
  Serial.println("eTVOC "+String(etvoc)+ " ppb");
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);  display.print("CO2 " + String(eco2)) ;
  display.setCursor(0,19); display.print("VOC " + String(etvoc)) ;
  display.setCursor(0,38); display.print("TMP " + FloatToStr(temperature,1)) ;
  display.setTextSize(1);
  display.setCursor(100,6);  display.print("ppm") ;
  display.setCursor(100,25); display.print("ppb") ;
  display.setCursor(10,56);  display.print(WiFilocalIP) ;
  display.display();
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {  // log values to web server
    LastLogMillis = millis();
    Serial.println("Log data of the average of last "+String(AVG_SIZE)+" measurements");
    add_CO2(ArrayAverage(AVG_eco2,  AVG_SIZE)) ;
    add_VOC(ArrayAverage(AVG_etvoc, AVG_SIZE)) ;
    add_TMP(temperature) ;
    CO2_plotvalues =  ArrayToString(CO2) ; // update the values for plotting  
    VOC_plotvalues =  ArrayToString(VOC) ; // update the values for plotting  
    TMP_plotvalues =  ArrayTempToString(TMP) ; // update the values for plotting  
    UpdateHTML();
  }  
}

String FloatToStr(float f,int d) {
  int decimal = round((f - int(f)) * pow(10,d)) ; 
  return String(int(f)) + "." + String(decimal) ;
}


void loop() {
  // Read
  
  sensors.requestTemperatures();  // read the temperature
  temperature = sensors.getTempCByIndex(0);
  Serial.println("Current temp = "+FloatToStr(temperature,1)+"°C  ") ;
  
  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2,&etvoc,&errstat,&raw); 
  if( errstat==CCS811_ERRSTAT_OK ) { 
    UpdateCCS811values(eco2, etvoc) ;
    
  } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
    Serial.println("CCS811: waiting for (new) data");
  } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
    Serial.println("CCS811: I2C error");
  } else {
    Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
    Serial.print("="); Serial.println( ccs811.errstat_str(errstat) ); 
  }
  server.handleClient();
  handle_root();
  // Wait
  delay(1000); 
}
