/*
  THIS CODE DOES NOT USE CS5490 LIBRARY, INSTEAD IT USES HardwareSerial 2 library,
  it means that it will be useful only for ESP32 AND SIMILAR BOARDS.
*/

#define REELAY_OFF 14
#define REELAY_ON 12
#define REELAY_LED 32

#define RGB_R 26
#define RGB_G 25
#define RGB_B 33

#define BUTTON 4

#define RST_SC5490 27

void readPushValues();

//**********-FOR BLYNK-********************
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
char auth[] = "AfUHvsOdH6UFhEV0FtIM_0IKUawP5HRl";
char ssid[] = "aalto open";
char pass[] = "";
//**********-FOR BLYNK END-****************

unsigned long time_now = 0;
int readPushDuration = 10000; // 3 min
const int readPushesQuantSet = 10;
int readPushesQuant = 10;

bool reelayOnOff = false;

bool buttonStateOld = false;
bool buttonSwitched = false;
bool BlynkButtonSwitched = false;
bool BlynkButtonStateOld = false;
bool RelayState = false;

void setup(){
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // wait for serial port to connect. Needed for Leonardo only
  while (!Serial);

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

  //"wakeuping" the chip
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
  Serial.println("LOOP");
  //**********-FOR BLYNK-********************
  Blynk.run();
  //**********-FOR BLYNK END-****************

  if((millis() > time_now + readPushDuration) || (0 < readPushesQuant)){ //every rad & push duration push the quantity of readPushesQuant
    if (millis() > time_now + readPushDuration){
      readPushesQuant = readPushesQuantSet;
    }
    time_now = millis();
    Serial.println("pushing vals.....");
    readPushValues();
    readPushesQuant--;
  }

  isButtonPressed();
  buttonSwitched = relaySwitch(buttonSwitched);
}

int readRmsV(){
  uint32_t value = readReg(16, 7);
  value = -74.1555 + 2 + value*4.053207*(pow(10,-5)); //Calibration function.
  if (value < 100 or value > 160){ //Out of realistic range (E.g. if no voltage is coming out of the soccet)
    value = 0;
    }
  delay(1000);
  return value;
 }

 int readRmsI(){
  uint32_t value = readReg(16, 6);
  value = 0.00000101078*1000*value + 0.00037*1000; //Calibrationfunc. Note the value is returned in no milliAmps!
  
  if (value < 11){ // zero values below 11 milliamps 
   value = 0;
    }
  
  delay(1000);
  return value;
 }

 float readPF(){
  uint32_t value = readReg(16, 21);
  Serial.println("raw");
  Serial.println(value);
  //float val = 0.5221002 - 7.52903E-8 * value + 1.16647E-14*(pow(value,(2)));  OLD
  // New func: y = 0.5228584 - 7.845135e-8*x + 1.291986e-14*x^2
  float val = 0.5228584 - 7.845135E-8 * value + 1.291986E-14*(pow(value,(2)));
  //The function is decent but still because of lack of data at value range 4*10^6 -> 0.98*10^7 it gets unsure values. Also note that under EU legislation no electrical devices can have a powerFactor under 0,95, therefore this calibration function should be accurate in normal device use.
  if (val > 1){
    val = 1;
  }
    
  if (val < 0.4){
   Serial.print("PF VAL UNDER 0,4 ?!!!");
   val = 0.4;
  }
  Serial.println("Not raw");
  Serial.println(val);
  delay(1000);
  return val;
 }
 
 float calcP(int V, int I, float PF){
  Serial.println("argumentae");
  Serial.println(V);
  Serial.println(I);
  Serial.println(PF);
  float P = V * I/(1000) * PF; //Where PF is cos(phase-displacement)
  return P;
 }

//********-instrument functions-**********
uint32_t readReg(int page, int adr){
  byte data[3]; //data buffer

  clearSerial2Buffer();

  Serial2.write(0b10000000 + page); //0b10000000 is added because of that how adress writing logic works. Read more in datasheet.
  Serial2.write(adr);

  //Wait for 3 bytes to arrive 
  while(Serial2.available() < 3);

  //Read 3 byte information
  for(int i=0; i<3; i++){
    data[i] = Serial2.read();
  }

  //Data concatenation
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
  RelayState = !(RelayState);
  if (RelayState){
    digitalWrite(REELAY_ON, HIGH);
    delay(1000); //later adjust to the minimum needed
    digitalWrite(REELAY_ON, LOW);
    digitalWrite(REELAY_LED, HIGH);
  }else{
    digitalWrite(REELAY_OFF, HIGH);
    delay(1000); //later adjust to the minimum needed
    digitalWrite(REELAY_OFF, LOW);
    digitalWrite(REELAY_LED, LOW);
    switched = !(switched);
    return switched;
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
BLYNK_WRITE(V0){ //may be needed to adjust "blunkbutttonstateold" to the state whichi is recieved during the first loop.
  bool BlynkButtonState = (bool)(param.asInt());
  if (BlynkButtonState != BlynkButtonStateOld){
    BlynkButtonStateOld = BlynkButtonState;
    buttonSwitched = true;
  }
}

void pushValueV(int val){
  if (val > 260 || val < 130){
  val = 0;
  }
  Blynk.virtualWrite(V5, val); //Vrites in "V5" Cloud's VirtualPin Register 
}

void pushValueI(int val){
  Blynk.virtualWrite(V4, val); 
}

void pushValueP(float val){
  Blynk.virtualWrite(V3, val);
}

void readPushValues(){
  int valV = readRmsV();
  
  int valI = readRmsI();
  
  float valPF = readPF();

  float valP = calcP(valV, valI, valPF);
  
  //**********-FOR BLYNK-********************
  pushValueV(valV); 
  pushValueI(valI/1000);
  pushValueP(valP);
  //**********-FOR BLYNK END-****************

  Serial.println("Rms Voltage:");
  Serial.println(valV, DEC);
  Serial.println("Rms Current:");
  Serial.println(valI, DEC);
  Serial.println("PF:");
  
  Serial.println("Power P = VIcos(u):");
  Serial.println(valP, DEC);
  Serial.println(valPF, DEC);
}