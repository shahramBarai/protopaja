/*
  1 - THIS EXAMPLE DOES NOT USE CS5490 LIBRARY, use only if you
  are having some trouble or need to understand how it works

  2 - THIS EXAMPLE USES HardwareSerial 2 library, it means that it
  will be useful only for ARDUINO MEGA and ESP32
*/

byte data[3]; //data buffer
const int resetpin = 27;

void setup(){
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  // wait for serial port to connect. Needed for Leonardo only
  while (!Serial);
  
  // set the data rate for the HardwareSerial 2 port
  Serial2.begin(600);

  //Restarting? idk. ask Kalle.
  pinMode(resetpin, OUTPUT);
  delay(2000);
  digitalWrite(resetpin, LOW);
  delay(2000);
  digitalWrite(resetpin, HIGH);

  //wakeuping the chip
  delay(2000);
  Serial2.write(0b11000011);

  //continuousConv
  delay(2000);
  Serial2.write(0b11010101);
}

void loop(){
  readRMSV();
  //readRmsI();
}

/* Clearing Serial2 Buffer */
void clearSerial2Buffer(){ //Dont understand the purpose of this but other coder did this every time before writing to Serial2
  while(Serial2.available()){
    Serial2.read();
  }
}

void readRMSV(){
  clearSerial2Buffer();
  Serial2.write(0b10010000); //Select Page 16 (adresses can be found in datasheet)
  Serial2.write(0b00000111); //Read Address 7
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

  value = -74.1555 + 2 + value*4.053207*(pow(10,-5)); //Calibration function.
  
  //Serial.println(value,HEX);
  Serial.println("Rms Voltage:");
  Serial.println(value,DEC);
  delay(1000);
 }

 void readRmsI(){
  clearSerial2Buffer();
  Serial2.write(0b10010000); //Select Page 16
  Serial2.write(0b00000110); //Read Address 7 
  //Wait for 3 bytes to arrive 
  while(Serial2.available() < 3);
  //Read 3 byte information
  for(int i=0; i<3; i++){
    data[i] = Serial2.read();
  }
  
  uint32_t value = 0;
  value = value + data[2] << 8;
  value = value + data[1] << 8;
  value = value + data[0];

  value = value / 1000; //the current value needs calibration!

  Serial.println("Rms Current:");
  Serial.println(value,DEC);
  delay(1000);
 }
