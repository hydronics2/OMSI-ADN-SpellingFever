



//    2/1/2015 - turned OFF master transmitting to Slave 4
//    2/1/2015 - ignore sending as 255 for rows 1,5,6,7,8,9... (it just looks buggy when they show up)
//    8/5/2015 - consider sending new data to slaves every 150ms (immediately after reading from PC)...... then perhaps requsting data from slaves




#include <Wire.h>

const int slavePowerRelay = 13;

byte testRows[14]; //initially gets rows to be lit from PC and tests whether unique
int logByte = 0; // logs each byte from Serail as available
int newMatrixFromPc = 0;

byte rows[14];  //rows to be lit (from PC via SerialRead)


byte pressed [14];  //to send to PC

byte pressed1[14];  //letters pressed on from slaves
byte pressed2[14];  //letters pressed on from slaves
byte pressed3[14];  //letters pressed on from slaves
byte pressed4[14];  //letters pressed on from slaves

byte pressedLast1[14];
byte pressedLast2[14];
byte pressedLast3[14];
byte pressedLast4[14];

int var;
byte serial_byteS[14]={33, 32,0,0,0,0,0,0,0,0,0,0,0,0};  //S
byte serial_byteI[14]={33, 0,0,0,0,0,0,0,1,0,0,0,0,0};  // I
byte serial_byteX[14]={33, 0,0,0,0,0,0,128,0,0,0,0,0,0}; // x
byte serial_byteF[14]={33, 0,0,0,0,0,0,0,0,0,0,0,0,32}; // Finish

byte trouble1[14];
byte trouble2[14];
byte trouble0[14];

long previousMillisSent = 0;
long previousMillisRequest = 0;
long previousMillisPc = 0;
long intervalPc = 150;  //lets collect PC data for 150 ms
long intervalSend = 200;  //intervals between i2c sending
long intervalRequest = 200;  //intervals between i2c requesting

int readingSerial = 0;
int readSerial = 0;
int numberFaults = 0;
int printReport = 0;  //prints faults report to Serial1

void setup() {
Serial.begin(9600);
Serial1.begin(9600);
Wire.begin(); 
TWBR = 152;      


}

void loop() {
  
unsigned long currentMillis = millis();


/*------------------------------------- SEND and RECEIVE DATA TO/FROM PC */

if(currentMillis - previousMillisPc < intervalPc ) {  //send/receive for 150ms every 200ms
// 

readingSerial = 1; //don't i2c anything while serial is reading

if(Serial.available() && logByte == 0) {
   testRows[0] = Serial.read();
   if(testRows[0] == 33){ //if true, read all 13 bytes
   logByte = 1; 
    }} 

if(Serial.available() >= 2 && logByte > 0) {  
   testRows[logByte] = Serial.read();
   logByte++; 

  if(logByte == 14) {  //the matrrix is complete, all 14 bytes have been received
  readSerial++;
  logByte = 0;          //configure ready to receive the 1st byte again (byte 33)
 //--------------------------------------SENDS rows to PC
  pressed[0] = 33;  
  Serial.write(pressed, 14); //the PC expects to receive this matrix from the master controller NOW
  
//   Serial1.println(pressed[0]);
//   Serial1.println(pressed[5]);
//   Serial1.println(pressed[6]);
//   Serial1.println(pressed[7]);
//   Serial1.println(pressed[8]);
//   Serial1.println(pressed[9]);
//   Serial1.println(pressed[10]);
//   Serial1.println(pressed[11]);
//   Serial1.println(pressed[12]);
//   Serial1.println(pressed[13]);
    if (memcmp(testRows, rows, 14) != 0) { //is the matrix new? has the PC has sent new data?
    memcpy(rows, testRows, 14);           //if yes, then copy new Matrix to rows
    numberFaults++;                       //number or unique testRows sent
    Serial1.println("unique packet from PC"); 
//    Serial1.println(testRows[0]);
//    Serial1.println(testRows[1]);
//    Serial1.println(testRows[2]);
//    Serial1.println(testRows[3]);
//    Serial1.println(testRows[4]);
//    Serial1.println(testRows[5]);
//    Serial1.println(testRows[6]);
//    Serial1.println(testRows[7]);
//    Serial1.println(testRows[8]);
//    Serial1.println(testRows[9]);
//    Serial1.println(testRows[10]);
//    Serial1.println(testRows[11]);
//    Serial1.println(testRows[12]);
//    Serial1.println(testRows[13]);

    newMatrixFromPc = 1;  // sets up an event to send new data to the slaves
    }
  }}
}
  
if(currentMillis - previousMillisPc > intervalPc && printReport == 1){                             // READING SERIAL time has elapsed
    printReport++;
    readingSerial = 0;
    memset(pressed, 0, 14); // clears the array
 } 
 
 
if(printReport == 2){ //reports on how many missed Serial.Reads therer were from the PC assuming the arrays should be the same if nothing has happened
  printReport++; // so it only prints the report once
 // Serial1.print("read this many serials: ");  //number of complete matricies read
 // Serial1.println(readSerial);
 // Serial1.print("number of Faults: ");  //number of matricies that didn't match the preceding matrix
 // Serial1.println(numberFaults);
  numberFaults = 0;
  readSerial = 0;
}

/////for the next 300ms.... wait for slaves to report buttons pressed  

if((currentMillis - previousMillisPc) > (intervalPc + 300)) {  //wait for 0.15 + .2 seconds before reading PC bytes
previousMillisPc = currentMillis; 
//Serial1.println("450ms is over, start logging"); 
while (Serial.read() != -1); //clears data in the PC Serial Port
logByte = 0;                //configure ready to receive the 1st byte again (byte 33)
printReport = 1;

}  




/*------------------------------------- SENDS DATA TO SLAVES */

if(currentMillis - previousMillisSent > intervalSend && newMatrixFromPc == 1 && readingSerial == 0) { //if the time period has elapsed AND new data is available AND we're not sending data serial, send to slaves
 previousMillisSent = currentMillis;  //restart the clock
 newMatrixFromPc = 0;  //dont retransmit unless new data has arrived
 Serial1.println("sending data to slaves");

  if(rows[1] != 255){        //don't turn them all on after a Finish
  Wire.beginTransmission(1); // transmit to device #1 
  Wire.write(rows[0]);              // sends one byte 
  Wire.write(rows[1]);
  Wire.write(rows[2]);
  Wire.write(rows[3]);
  Wire.write(rows[4]);
  if (Wire.endTransmission () == 0) {
        Serial1.println("master sent x to Slave 1");
  }
      else {
         Serial1.println("master did not send to Slave 1");

      }
  }
  delay (10);
  
  if(rows[5] != 255  && rows[6] != 255 && rows[7] != 255 && rows[8] != 255){
  Wire.beginTransmission(2); // transmit to device #2 
  Wire.write(rows[0]);              // sends one byte 
  Wire.write(rows[5]);
  Wire.write(rows[6]);
  Wire.write(rows[7]);
  Wire.write(rows[8]);
  if (Wire.endTransmission () == 0){
         Serial1.println("master sent x to Slave 2"); 
  }
      else {
        Serial1.println("master did not send to Slave 2");

      }
  }
  
  delay (10);
  
  if(rows[9] != 255){
  Wire.beginTransmission(3); // transmit to device #3 
  Wire.write(rows[0]);              // sends one byte 
  Wire.write(rows[9]);
  Wire.write(rows[10]);
  Wire.write(rows[11]);
  Wire.write(rows[12]);
  if (Wire.endTransmission () == 0) {
        Serial1.println("master sent x to Slave 3"); 
  }
      else {
   Serial1.println("master did not send to Slave 3");

      }
  }
        delay (10);
  
  
//  Wire.beginTransmission(4); // transmit to device #4    
//  Wire.write(rows[13]);
//  if (Wire.endTransmission () == 0) {
//    //    Serial1.println("master sent x to Slave 4"); 
//  }
//      else {
//     //   Serial1.println("master did not send to Slave 4");
//      }
//  delay (10);


}



/*------------------------------------- REQUEST DATA FROM SLAVES */

if(currentMillis - previousMillisRequest > intervalRequest && readingSerial == 0) { //if the time period has elapsed request data from slaves
 previousMillisRequest = currentMillis;  //restart the clock

/*-------------------------------------REQUEST FROM SLAVE 1 */
  if(Wire.requestFrom(1, 14) == 14) {    // request letter from Slave(1) consisting 14 bytes 
    for(int i=0; i<14; i++){
    pressed1[i] = Wire.read(); 
   
    //Serial1.println(pressed1[i]); 
    }

  if(pressed1[1] != pressedLast1[1] || pressed1[2] != pressedLast1[2] || pressed1[3] != pressedLast1[3] || pressed1[4] != pressedLast1[4]){
    memcpy(pressed, pressed1, 14 );           //copies to pressed array to be sent to PC
    memcpy(pressedLast1, pressed1, 14 );      //copies last pressed to pressed One
   Serial1.println("master received unique packet from Slave 1");

    }
    }  

   else {
      Serial1.println("does not recieve data from slave 1"); 
        }
  
  delay(10);
  
/*-------------------------------------REQUEST FROM SLAVE 2 */
  if(Wire.requestFrom(2, 14) == 14) {    // request letter from Slave(2), SECTION 2 consisting 14 bytes 
  for(int i=0; i<14; i++){
  pressed2[i] = Wire.read(); 

  //Serial1.println(pressed2[i]); 
  }
  if(pressed2[5] != pressedLast2[5] || pressed2[6] != pressedLast2[6] || pressed2[7] != pressedLast2[7] || pressed2[8] != pressedLast2[8]){
    memcpy( pressed, pressed2, 14 );
    memcpy( pressedLast2, pressed2, 14 );
   Serial1.println("master received unique packet from Slave 2");
    
  }
  }
     else {
     Serial1.println("does not recieve data from slave 2"); 
          }
  
  delay(10);
  
  /*-------------------------------------REQUEST FROM SLAVE 3 */
  if(Wire.requestFrom(3, 14) == 14) {    // request letter from Slave(2), SECTION 2 consisting 14 bytes 
  for(int i=0; i<14; i++){
  pressed3[i] = Wire.read(); 

  //Serial1.println(pressed3[i]); 
  }
  if(pressed3[9] != pressedLast3[9] || pressed3[10] != pressedLast3[10] || pressed3[11] != pressedLast3[11] || pressed3[12] != pressedLast3[12]){
    memcpy( pressed, pressed3, 14 );
    memcpy( pressedLast3, pressed3, 14 );
    Serial1.println("master received unique packet from Slave 3");
   
  }
  }
     else {
     Serial1.println("does not recieve data from slave 3"); 
          }
  delay(10);
  
  /*-------------------------------------REQUEST FROM SLAVE 4 */
  if(Wire.requestFrom(4, 14) == 14) {    // request letter from Slave(4), SECTION 4 consisting 14 bytes 
  for(int i=0; i<14; i++){
  pressed4[i] = Wire.read(); 

  //Serial1.println(pressed4[i]); 
  }
  if(pressed4[13] != pressedLast4[13]){
    pressed[13] = pressed4[13];
    Serial1.print("got new row 13 finish line!!!!!!!!!!!!!!!!!!!!!!!!1");
    memcpy( pressed, pressed4, 14 );
    memcpy( pressedLast4, pressed4, 14 );
   Serial1.println("master received unique packet from Slave 4");
  
  }
  }
     else {
     Serial1.println("does not recieve data from slave 4"); 

          }
  delay(10);
}


}



