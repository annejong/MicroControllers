/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
  Solar Dc 12V 3M Warmtapwatercirculatiepomp Borstelloze Motor Waterpomp Mini Elektrische Dompelpomp Water pomp Food Grade
  
*********/


// PIN CONNECTIONS; Motor speed control L298N
// Motor need 12V Powersupply; connect 12V and GND to adaptor
// L298N has 6 pins starting at the 5V connection; EMA, IN1, IN2, IN3, IN4, ENA(EnableB)
int motorA_IN1 = 32; // IN1
int motorA_IN2 = 33; // IN2
int motorA     = 25; // ENA


// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
const int minSpeed = 100;  // increase this if the motor does not start

void setup() {
  // sets the pins as outputs:
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorA, OUTPUT);
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(motorA, pwmChannel);

  Serial.begin(9600);

}

void loop() {
  // Move the DC motor forward at maximum speed
  Serial.println("MotorA full speed");
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH); 
  delay(2000);


  //  DC motor forward with increasing speed
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH);
  int Speed = minSpeed; 
  while (Speed <= 255){
    ledcWrite(pwmChannel, Speed);   
    Serial.println("Speed: " + String(Speed));
    Speed = Speed + 10;
    delay(500);
  }
}
