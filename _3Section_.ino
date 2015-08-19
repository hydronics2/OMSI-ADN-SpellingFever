/*
 Shift OUT 75HC595
//PINS 1-7, 15	Q0 " Q7	Output Pins
//PIN 8	GND	Ground, Vss
//PIN 9	Q7"	Serial Out                                      ARDUINO D11
//PIN 10	MR	Master Reclear, active low              Goes to VCC
//PIN 11	SH_CP	Shift register clock pin                ARDUINO D13
//PIN 12	ST_CP	Storage register clock pin (latch pin)  ARDUNIO D4
//PIN 13	OE	Output enable, active low               Tied to ground
//PIN 14	DS	Serial data input                       Goes to next shift chip (Q7)
//PIN 16	Vcc	Positive supply voltage

*/

//To do:
//1) don't write lights unless rows(0) is new from master or 1 second has passed
//2) If nothing happens for 5 minutes... go into attractor mode....
//3) 


//   1/31/2015 - after 3.5 seconds, the arrays go to zero

#include <Wire.h>

#define number_of_74hc595s 4 

#define numOfShiftOutPins number_of_74hc595s * 8


int latchPin = 8; // Connects to /PL aka Parallel load aka latch pin on the 74HC165
int clockEnablePin  = 9;  // Connects to Clock Enable pin the 165 (should always be low)
int dataPin = 10;  // Connects to the Q7 pin on the 74HC165 (MISO for SPI)
int clockPin = 12;  // Connects to the Clock pin on the 74HC165


int SER_Pin = 11;   //pin 14 on the 75HC595 DS Serial data input
int RCLK_Pin = 4;  //pin 12 on the 75HC595  ST_CP shift register clock input
int SRCLK_Pin = 13; //pin 11 on the 75HC595  SH_CP shift register clock input

boolean registers[32];  //Shift out states




//Define variables to hold the data 
//for each shift register.
byte switchVar1; 
byte OLDswitchVar1;
byte switchVar2;
byte OLDswitchVar2;
byte switchVar3;
byte OLDswitchVar3;
byte switchVar4;
byte OLDswitchVar4;



byte serial_byte1[14]={33, 32,0,0,0,0,0,0,0,0,0,0,0,0};  //S
byte serial_byte2[14]={33, 0,32,0,0,0,0,0,0,0,0,0,0,0};  // E
byte serial_byte3[14]={33, 0,0,32,0,0,0,0,0,0,0,0,0,0}; // D
byte serial_byte4[14]={33, 0,0,0,64,0,0,0,0,0,0,0,0,0}; // C

byte bytesToSend[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

volatile byte rows[5];
volatile int waitToSend = 0;  //this delay after a letter has been switched so it can be sent to the master


long previousMillis = 0;
long timeSenseNewLetter = 0; //last instance a letter was stepped on




int var = 48;
byte mask = 1; //bitmask for going from rows to lights
boolean lights[32];  //lights, letters to be lit

int buttonState = 0;


void setup()
{
  Wire.begin(3);                // join i2c bus with address #3
    TWBR = 152;  //set wire bus speed at 50khz
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent);  //sends Master buttons pressed
  Serial.begin(9600);           // start serial for output

  //Shift IN/OUT pieces
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT); 
  pinMode(dataPin, INPUT);
  pinMode(clockEnablePin, OUTPUT);
  digitalWrite(clockEnablePin, LOW);
  digitalWrite(latchPin, HIGH);
  digitalWrite(clockEnablePin, LOW);   
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);
  //reset all register pins
  clearLights();
  writeLights();

}

void clearLights(){
  for(int i = numOfShiftOutPins - 1; i >=  0; i--){
     lights[i] = LOW;
  }
} 




void loop()
{
unsigned long currentMillis = millis();


  
if(waitToSend == 0) {  //sends previous bytes before gathering more
  
  digitalWrite(latchPin, LOW);
  delayMicroseconds(5);
  digitalWrite(latchPin, HIGH);
  
  switchVar1 = shiftIn(dataPin, clockPin);
  switchVar2 = shiftIn(dataPin, clockPin);
  switchVar3 = shiftIn(dataPin, clockPin);
  switchVar4 = shiftIn(dataPin, clockPin);
}


if (switchVar1 != OLDswitchVar1 || switchVar2 != OLDswitchVar2 || switchVar3 != OLDswitchVar3 || switchVar4 != OLDswitchVar4)
{
   Serial.println("switched");

   //display_switch_values();

   determineButtonState();
    
   OLDswitchVar1 = switchVar1;
   OLDswitchVar2 = switchVar2;
   OLDswitchVar3 = switchVar3;
   OLDswitchVar4 = switchVar4;
    
   if(buttonState == 1){      //only send to Master if button state is HIGH (letter has been stepped on).... this just decreases to i2c traffic
   Serial.println("button state is 1");
   memset(bytesToSend, 0, 14); // clears the array
   bytesToSend[0] = 33;
   bytesToSend[9] = switchVar1;
   bytesToSend[10] = switchVar2;
   bytesToSend[11] = switchVar3;
   bytesToSend[12] = switchVar4;
   
   timeSenseNewLetter = currentMillis;
   waitToSend = 1; //sets 'Wait to send' flag before accumulating more data
   buttonState = 0;
   }
   
}


//--------------------------------------------------------------CLEAR OLD ARRAY
if(currentMillis - timeSenseNewLetter > 3500){  //if 3.5 seconds has gone by sense a new letter has been stepped on, set the array to zero
memset(bytesToSend, 0, 14);} // clears the array
  
  
//delay so all these print satements can keep up. 
delay(20); 
  
  
if(rows[0] == 33){
maskRowsToLights(); //writes rows to 32 letters to be lit
writeLights();  //lights letters
}
//serialPrintRowsLights();  //for debugging

}
//-----------------------------------------------END OF LOOP


//------------------------------------------------ GET Rows from Master

void receiveEvent(int howMany)  //executes when Master sends rows (letters to be lit)
{
  if(Wire.available() > 4) 
  {
    for(int i=0;i<5;i++){          //read 4 bytes - rows 1-4
      rows[i]=Wire.read(); 
    }
//Serial.println("received 4 rows from master");
    }
}


//------------------------------------------------ SEND Rows to Master

void requestEvent()  //sends switch stepped on to Master when requested from Master
{
  Wire.write(bytesToSend, 14); 
  waitToSend = 0; //resets 'wait till sent' delay
//Serial.println(bytesToSend[0]);
//Serial.println(bytesToSend[1]);
//Serial.println(bytesToSend[2]);
//Serial.println(bytesToSend[3]);
//Serial.println(bytesToSend[4]);

}


//-----------------------------------------------------SHIFT OUT

void maskRowsToLights(){  //masks the 4 rows of bytes to 32 bits (lights)
int j = 0;
for(int i=1;i<5;i++){
for(mask = 00000001; mask > 0; mask <<=1){
  if(rows[i] & mask){  //if bitwise AND resolves to true
  lights[j] = 1;}
  else{
    lights[j] = 0;}
    j++;
}}}

//Only call AFTER all values are set how you would like (slow otherwise)

void writeLights(){  //lights of letters
  digitalWrite(RCLK_Pin, LOW);
  for(int i = numOfShiftOutPins - 1; i >=  0; i--){
    digitalWrite(SRCLK_Pin, LOW);
    int val = lights[i];
    digitalWrite(SER_Pin, val);
    digitalWrite(SRCLK_Pin, HIGH);
  }
  digitalWrite(RCLK_Pin, HIGH);
}


//----------------FUNCTION NOT USED
void serialPrintRowsLights(){
int j = 0;

for(int i=1;i<5;i++){
for(int k=0; k<8; k++){
  Serial.print("row ");
  Serial.print(i);
  Serial.print(": ");
  Serial.print(rows[i]);  
  Serial.print(", lights ");
  Serial.print(j);
  Serial.print(": ");
  Serial.println(lights[j]);
    j++;
}
}
}

//-------------------------------------------------------SHIFT IN

byte shiftIn(int myDataPin, int myClockPin) { 
  int i;
  int temp = 0;
  int pinState;
  byte myDataIn = 0;

  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, INPUT);

  for (i=7; i>=0; i--)
  {
    digitalWrite(myClockPin, 0);
    delayMicroseconds(2);
    temp = digitalRead(myDataPin);
    if (temp) {
      
      pinState = 1;
      myDataIn = myDataIn | (1 << i);
    }
    digitalWrite(myClockPin, 1);
  }
  return myDataIn;
}

void switchesToBytes(){

}




//------------------------------------------------DETERMINE BUTTON STATES

void determineButtonState()  //determines if the change-of-state is high or low
{
  int j = 8;  
  
    for(int i = 0; i <= 7; i++)
    {

      if((switchVar1 >> i) & 1) {
            //Serial.println(i);
            buttonState = 1;
            registers[i] = 1; }    //writes register array values (0-7) based on button state
        else {
            registers[i] = 0; }    //writes register array values (0-7) based on button state
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar2 >> i) & 1) {
            //Serial.println(j);
            buttonState = 1;
            registers[j] = 1; }   //writes register array values (8-15) based on button state
        else {
            registers[j] = 0; }   //writes register array values (8-15) based on button state
      j++;
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar3 >> i) & 1) {
            //Serial.println(j);
            buttonState = 1;
            registers[j] = 1; }   //writes register array values (16-23) based on button state
        else {
            registers[j] = 0; }   //writes register array values (16-23) based on button state
      j++;
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar4 >> i) & 1) {
            //Serial.println(j);
            buttonState = 1;
            registers[j] = 1; }   //writes register array values (24-31) based on button state
        else {
            registers[j] = 0; }   //writes register array values (24-31) based on button state
      j++;
      }
      
     }



//------------------------------------------------DISPLAY SWITCH VALUES
void display_switch_values()
{
  int j = 8;  
  
    for(int i = 0; i <= 7; i++)
    {

      if((switchVar1 >> i) & 1) {
            Serial.println(i);
            registers[i] = 1; }    //writes register array values (0-7) based on button state
        else {
            registers[i] = 0; }    //writes register array values (0-7) based on button state
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar2 >> i) & 1) {
            Serial.println(j);
            registers[j] = 1; }   //writes register array values (8-15) based on button state
        else {
            registers[j] = 0; }   //writes register array values (8-15) based on button state
      j++;
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar3 >> i) & 1) {
            Serial.println(j);
            registers[j] = 1; }   //writes register array values (16-23) based on button state
        else {
            registers[j] = 0; }   //writes register array values (16-23) based on button state
      j++;
      }
    for(int i = 0; i <= 7; i++)
    {
       if((switchVar4 >> i) & 1) {
            Serial.println(j);
            registers[j] = 1; }   //writes register array values (24-31) based on button state
        else {
            registers[j] = 0; }   //writes register array values (24-31) based on button state
      j++;
      }
      
     }
//------------------------------------------------end print loop


