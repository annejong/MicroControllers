/*
  ESP32 Temperature Web Server - STA Mode
  modified by Anne de Jong

  When using the ESP32 with Arduino IDE, 
  the default I2C pins are GPIO 22 (SCL) and GPIO 21 (SDA) 
  but you can configure your code to use any other pins.

*/

#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SSID & Password
const char* ssid = "Hartelijk";  // Enter your SSID here
const char* password = "Welkomja123";  //Enter your Password here
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

// PIN CONNECTIONS
const int oneWireBus = 14;  // GPIO 14 pin is used to read the data from the DS18B20 (yellow wire)    
                            // DS18B20 Board connections: red 3-5V;  black GND;  yellow, GPIO 14 and 4.7kOhm to 3-5V

// SSD1306 OLED display display connected to I2C (SDA_21, SCL_22 pins); 
// connectors=GND;VCC;SCL;SDA ==> gnd; 3.3V; GPIO22; GPIO21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Digital thermometer
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 


int LOG_INTERVAL = 6 ;// log sample time in seconds
unsigned long LastLogMillis ; 
const int maxXY=200 ;         // number of measurements in graph
float y[maxXY] ;              // Temperature values 
String HTML = "";
String strYvalues = "";
String strXvalues ;
float maxTemp ;
float minTemp ;


void setup() {
  Serial.begin(9600);
  Serial.println("Try Connecting to ");
  Serial.println(ssid);

  // Connect to your wi-fi modem
  WiFi.begin(ssid, password);

  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial

  server.on("/", handle_root);
  server.begin();
  Serial.println("HTTP server started");
  delay(100); 

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


  // Start the DS18B20 digital temp sensor
  sensors.begin(); 

  LastLogMillis = millis();  //initial start time
  // Make xValues string
  strXvalues = '[' ;
  for (int i = 0; i < maxXY; i++) {  strXvalues += String(i)+","; }  
  strXvalues += String(maxXY) + ']' ; // add the last without comma
  // set all initial temp values to 20°C
  for (int i = 0; i <= maxXY; i++) {  y[i] = 20; }  
  
}


void UpdateHTML(float temperature ) {
  // HTML & CSS contents which display on web server
  HTML = "<html>" ;
  HTML += "<script src=https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js></script>";
  HTML += "<body><head><meta http-equiv=refresh content=15></head><h1>Hondsrugbier ESP32 - Temperatuur Webserver</h1>" ;
  HTML += "<canvas id=myChart style=width:100%;max-width:1000px></canvas>" ;
  HTML += "Current temperature = "+String(temperature)+"&#176;C<br>";
  HTML +="<a href=data:application/octet-stream,"+strYvalues+">Download data</a>";
  // HTML += "<br><br>Temp values<hr><br>"+strYvalues+"<br><hr>" ;
  // Add the chart.js script
  HTML += "\
  <script>                                                     \
    var xValues = "+strXvalues+";      \
    var yValues = "+strYvalues+";                  \
    new Chart(\"myChart\", {                                       \
      type: \"line\",                                              \
      data: {                                                    \
        labels: xValues,                                         \
        datasets: [{                                             \
          fill: false,                                           \
          lineTension: 0,                                        \
          backgroundColor: \"rgba(0,0,255,1.0)\",                  \
          borderColor: \"rgba(0,0,255,0.1)\",                      \
          data: yValues                                          \
        }]                                                       \
      },                                                         \
      options: {                                                 \
        legend: {display: false},                                \
        scales: {                                                \
          xAxes: [{                                              \
            scaleLabel: {display: true,  labelString: 'Time in minutes after sampling'} \
          }],  \
          yAxes: [{                                              \
            ticks: {min: "+String(round(minTemp))+", max:"+String(round(maxTemp))+"}, \
            scaleLabel: {display: true,  labelString: 'Temperature in °C'} \
          }]  \
        }                                                        \
      }                                                          \
    });                                                          \
  </script> \
  ";
    
  HTML += "</body></html>";
}

void add_temp(float t) {
  strYvalues = "[";
  for (int i = 0; i < maxXY-1; i++) {
    y[i] = y[i+1] ; 
    strYvalues += String(y[i]) + "," ; 
  }
  y[maxXY-1] = t ;
  strYvalues += String(t) + "]" ; 
}

void add_temp_reverse(float t) {
  y[maxXY-1] = t ;
  strYvalues = "[";
  for (int i = 0; i < maxXY-1; i++) {
    y[i] = y[i+1] ; 
    strYvalues += String(y[maxXY-1-i  ]) + "," ; 
  }
  strYvalues += String(y[0]) + "]" ; 
}

String FloatToStr(float f,int d) {
  int decimal = round((f - int(f)) * pow(10,d)) ; 
  return String(int(f)) + "." + String(decimal) ;
}

void OLED_temperature(String S) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(35,0);
  display.print(S);
  display.print((char)247); // degree symbol 
  display.println("C");
  OLED_plot() ;
  display.display();
}  

void OLED_plot() {
  // OLED is 128 pixels, so only plot first 100 temp values
  int xRange = 100 ; // 100 pixels
  int yRange = 50 ; // 50 pixels
  float yMin = minTemp +1  ;  // Here I use +/- 1 °C while the webserver limits are +/- 2°C
  float yMax = maxTemp -1  ;
  float scale = yRange / (yMax - yMin) ;
  float median = (yMax + yMin)/2 ;
  int medianScale = int((median -yMin) * scale); 
  for (int i = 0; i < xRange; i++) {  
    int TempScale = int((y[maxXY-i-1] -yMin) * scale);  // Add the last values
    display.drawPixel(128-i, 64-TempScale, WHITE);
  }
  for (int i = 0; i < xRange; i=i+3) {  // draw a dotted median line
    display.drawPixel(128-i, 64-medianScale, WHITE);
  }  
  display.setTextSize(1);
  display.setCursor(0,19);
  display.print(FloatToStr(yMax,1)) ;
  display.setCursor(0,54);
  display.print(FloatToStr(yMin,1)) ;  
  display.setCursor(0,36);
  display.print(FloatToStr(median,1)) ;  
 
}

void loop() {
  sensors.requestTemperatures();  // read the temperature
  float temperature = sensors.getTempCByIndex(0);
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {
    LastLogMillis = millis();
    add_temp_reverse(temperature) ;
    maxTemp = -273 ;
    minTemp = +273 ;
    for (int i = 0; i < maxXY; i++) { maxTemp = max(maxTemp, y[i]);  minTemp = min(minTemp, y[i]); }   // get temp limits
    maxTemp = maxTemp+2 ;
    minTemp = minTemp-2 ;
    UpdateHTML(temperature);
    Serial.println("Temperature value send to web server= "+String(temperature)+"°C  ") ;
  }    
  Serial.println("Current temp = "+FloatToStr(temperature,1)+"°C  ") ;
  
  server.handleClient();
  handle_root();

  OLED_temperature(FloatToStr(temperature,1)) ;
  delay(3000) ;  // 3 seconds
}



// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}
