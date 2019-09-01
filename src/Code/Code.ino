/*********************************************************************************************************************************************
  Aalto protopaja summer 2019 in collaboration with Riot innovations.
  
  Code runs with an esp-32 - wifi module using "Blynk" as an user interaction app, which uses its own cloud. 
  (Note that all functions which use blynk are marked so that in future it would be easier to be able to possibly change to another framework.)
  
  Before installing code remember to switch devise to boot mode by pressing boot-button for 10 seconds while plugging the device into socket.
  For operation remember to restart device by turning it off and on after loading new code.

  Copyright (c) . All rights reserved. Licensed under the MIT License.
**********************************************************************************************************************************************/

#define REELAY_OFF 14
#define REELAY_ON 12

#define REELAY_LED 32

#define RGB_R 26
#define RGB_G 25
#define RGB_B 33

#define BUTTON 4 //for on-device -button implementations. Check isButtonPressed() -function, (used in loop)

#define RST_SC5490 27 

//**********-FOR BLYNK-********************
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
char auth[] = "YOUR AUTH. TOKEN";
char ssid[] = "YOUR WIFI";
char pass[] = "YOUR PASSWORD";
//**********-FOR BLYNK END-****************

float readPushValues();

unsigned long time_now = 0;
int readPushDuration = 10000; // = 10 sec, time gap between reading values from an energy meter chip.
const int readPushesQuantSet = 10;
int readPushesQuant = 10;

float energyCost = 0; //In euros
float totalEnergyCost = 0; //In euros

bool reelayOnOff = false;

bool buttonStateOld = false;
bool buttonSwitched = false;
bool BlynkButtonSwitched = false;
bool BlynkButtonStateOld = false;
bool RelayState = false;

void setup(){
  //Serial.begin(115200); //Uncomment theese for serialport communication with computers serial monitor.
  // wait for serial port to connect.
  //while (!Serial);

  // set the data rate for the HardwareSerial 2 port
  Serial2.begin(600);

  pinMode(REELAY_OFF, OUTPUT);
  pinMode(REELAY_ON, OUTPUT);
  pinMode(REELAY_LED, OUTPUT);

  //Restarting chip
  pinMode(RST_SC5490, OUTPUT);
  delay(2000);
  digitalWrite(RST_SC5490, LOW);
  delay(2000);
  digitalWrite(RST_SC5490, HIGH);

  //waking up the chip
  delay(2000);
  Serial2.write(0b11000011);

  //continuousConv.
  delay(2000);
  Serial2.write(0b11010101);

  //**********-FOR BLYNK-********************
  Blynk.begin(auth, ssid, pass);
  //**********-FOR BLYNK END-****************
}

void loop(){
  //**********-FOR BLYNK-********************
  Blynk.run();
  //**********-FOR BLYNK END-****************

  if((millis() > time_now + readPushDuration) || (0 < readPushesQuant)){ //every rad & push duration push the quantity of readPushesQuant
    if (millis() > time_now + readPushDuration){
      readPushesQuant = readPushesQuantSet;
    }
    time_now = millis();
  
  //Calculate next power interval
  //w*€/kwh*second
  totalEnergyCost += ((float)readPushValues() * (float)energyCost/3600000.0 * (((float)millis()-(float)time_now)/1000.0) );
  readPushesQuant--;
  }

  //isButtonPressed(); //Uncomment, if you plan to use on-device button.
  buttonSwitched = relaySwitch(buttonSwitched);
}

int readRmsV(){
  uint32_t value = readReg(16, 7);
  value = -74.1555 + 2 + value*4.053207*(pow(10,-5)); //Calibration function.
 
  if (value < 100 or value > 260){ //Out of realistic range (E.g. the case if no voltage is coming out of the soccet)
    value = 0;
  }
    
  delay(1000);

  return value;
 }

 int readRmsI(){
  uint32_t value = readReg(16, 6);
  value = 0.00000101078*1000*value + 0.00037*1000; //Calibrationfunc. Note the value is returned in milliAmps!
  
  if (value < 40){ //zero all values below 40 milliamps (out of range)
   value = 0;
  }
    
  delay(1000);

  return value;
 }

 float readPF(){
  uint32_t value = readReg(16, 21);

  float val = 0.5228584 - 7.845135E-8 * value + 1.291986E-14*(pow(value,(2)));
  //This function is decent but still because of lack of data at value range 4*10^6 -> 0.98*10^7 it gets unsure values. Also note that under EU legislation no electrical devices can have a powerFactor under 0,95, therefore this calibration function should be very accurate in normal device usage.
  
  if (val > 1){ //No power factor can be greater than 1.
    val = 1;
  }
    
  if (val < 0.4){ //Unrealistic PF.
   val = 0.4;
  }

  delay(1000);

  return val;
 }
 
 float calcP(int V, int I, float PF){
  float P = V * I/(1000) * PF; //Where PF is cos(phase-displacement)
  return P;
 }

//********-instrumentary functions-**********
uint32_t readReg(int page, int adr){
  byte data[3]; //data buffer

  clearSerial2Buffer();

  Serial2.write(0b10000000 + page); //0b10000000 is added because of that how adress writing logic works. Read more in datasheet.
  Serial2.write(adr);

  //wait for 3 bytes to come 
  while(Serial2.available() < 3);

  //read 3 byte info
  for(int i=0; i<3; i++){
    data[i] = Serial2.read();
  }

  //combining data
  uint32_t value = 0;
  value = value + data[2] << 8;
  value = value + data[1] << 8;
  value = value + data[0];
  return value;
}

void clearSerial2Buffer(){ //Clears Serial2 buffer every time before writing to Serial2
  while(Serial2.available()){
    Serial2.read();
  }
}

bool relaySwitch(bool switched){
  if(switched){ 
    RelayState = !(RelayState);

    if (RelayState){
      digitalWrite(REELAY_ON, HIGH);

      delay(1000); 

      digitalWrite(REELAY_ON, LOW);
      digitalWrite(REELAY_LED, HIGH);

      switched = !(switched);

      return switched;
    }
    
    else{
      digitalWrite(REELAY_OFF, HIGH);

      delay(1000);

      digitalWrite(REELAY_OFF, LOW);
      digitalWrite(REELAY_LED, LOW);

      switched = !(switched);

      return switched;
    }
  }
}

void isButtonPressed(){
  bool buttonState = (bool)(digitalRead(BUTTON));

  if (buttonState != buttonStateOld){
    buttonStateOld = buttonState;
    buttonSwitched = true;
  }
}

//**********-BLYNK FUNCTIONS->************
//Blynk virtual pins usage:
//V1 => Energy cost from Blynk
//V6 => Total Energy spent from Blynk
//V5 => Voltage
//V4 => Current 
//V3 => Power

void pushValueC(int val){
  Blynk.virtualWrite(V6, val);
}

BLYNK_WRITE(V1){
  energyCost = (float)(param.asFloat());
}

BLYNK_WRITE(V0){ 
  bool BlynkButtonState = (bool)(param.asInt());

  if (BlynkButtonState != BlynkButtonStateOld){
    BlynkButtonStateOld = BlynkButtonState;
    buttonSwitched = true;
  }
}

void pushValueV(int val){
  Blynk.virtualWrite(V5, val); //Vrites in "V5", a cloud's VirtualPin Register 
}

void pushValueI(int val){
  Blynk.virtualWrite(V4, val); 
}

void pushValueP(float val){
  Blynk.virtualWrite(V3, val);
}

float readPushValues(){
  int valV = readRmsV();
  
  int valI = readRmsI();
  
  float valPF = readPF();

  float valP = calcP(valV, valI, valPF);
  
  //**********-FOR BLYNK-********************
  pushValueV(valV); 
  pushValueI(valI/1000);
  pushValueP(valP);
  pushValueC(totalEnergyCost);
  //**********-FOR BLYNK END-****************

  return valP;
}