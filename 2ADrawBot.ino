/****************************** 
 * Two Axis Draw Bot driver; Created by Dan Kelley
 * 
 * Accelstepper / MultiStepper library is Copyright (C) 2010 Mike McCauley
 *   more details may be found here: http://www.airspayce.com/mikem/arduino/AccelStepper/
*******************************/

// PLEASE CHANGE stepsPerMM variable measurements with your own values to avoid issues.
//  in fact.. please check over most variables, and the rest of the code if you can. I'd love some help.
//
// Using this code is at your own risk, and I take no responsibility for what you do with it, or if you break your stuff.
// This sketch requires my small cmd.h commandBuffer Library. I'll edit and add comments on this as I work some small bugs.

#include <cmd.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

//IO pins
#define PIN_INPUT1 A0
#define PIN_INPUT2 A1
#define PIN_INPUT3 A2
#define PIN_INPUT4 A3
#define PIN_INPUT5 A6
#define M_ON 0
#define M_OFF 1
#define PIN_M1Enable 10
#define PIN_M1Step 9
#define PIN_M1Dir 8
#define PIN_M2Enable 7
#define PIN_M2Step 6
#define PIN_M2Dir 5

//Serial Vars
String serialString = "";
CommandBuffer cmdBuffer;
bool sendRequest = true;

//Speeds
float stepperSpeed = 400;
int speed[2] = {stepperSpeed,stepperSpeed};


//Measurement vars
//X Motor is a 1.8' nema 17 with GT2 belt and 16t pulley
//Y Motor is an 18' mini geared stepper at a 1/36 reduction (same pulley)
float stepsPerMM[2] = {50.0,22.5};
bool hasLimits = false;
float input_limitX = 400;
float input_limitY = 200;
float limitX = 0;
float limitY = 0;

bool hasSlop = false;
float input_slopX = 10;
float input_slopY = 10;
float slopX = 0;
float slopY = 0;
bool xDir = false;
bool yDir = false;


long endStop = 9999999999;
long startStop = -9999999999;

AccelStepper stepperY(AccelStepper::DRIVER, PIN_M1Step, PIN_M1Dir);
AccelStepper stepperX(AccelStepper::DRIVER, PIN_M2Step, PIN_M2Dir);

MultiStepper steppers;
long positions[2] = {0,0}; // Array of desired stepper positions

void setup()
{
  Serial.begin(9600);
  
//MOTOR 1
  pinMode(PIN_M1Enable, OUTPUT);  //#PIN_ENABLE
  digitalWrite(PIN_M1Enable, M_ON);

//MOTOR 2 
  pinMode(PIN_M2Enable, OUTPUT);  //#PIN_ENABLE
  digitalWrite(PIN_M2Enable, M_ON);

//INPUTS
  pinMode(PIN_INPUT1, INPUT_PULLUP);  //#PIN_0
  pinMode(PIN_INPUT2, INPUT_PULLUP);  //#PIN_1
  pinMode(PIN_INPUT3, INPUT_PULLUP);  //#PIN_2
  pinMode(PIN_INPUT4, INPUT_PULLUP);  //#PIN_3
  pinMode(PIN_INPUT5, INPUT);  //#PIN_6

  stepperY.setMaxSpeed(stepperSpeed);
  stepperY.setAcceleration(5000);
  stepperY.setEnablePin(PIN_M1Enable);
  stepperY.setPinsInverted(true,false,true);
  stepperY.disableOutputs();
  
  stepperX.setMaxSpeed(stepperSpeed);
  stepperX.setAcceleration(5000);
  stepperX.setEnablePin(PIN_M2Enable);
  stepperX.setPinsInverted(false,false,true);
  stepperY.disableOutputs();

  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);
  recalculateDistance();
  delay(3000);
}

void loop(){
  requestCommand();
  readSerial();
  runCommandsFromBuffer();
}

/////////////////////////////////////////
//FEATURE FUNCTIONS
/////////////////////////////////////////

void parseCommand(){
  //Serial.println("parsing cmd");
  String cmd[3];
  cmdBuffer.deQueue(cmd[0], cmd[1], cmd[2]);
  
  cmd[0].toUpperCase();
  
  //Move absolute
  if (cmd[0] == "MA"){
    startMovement(false, cmd[1].toInt(), cmd[2].toInt());
  }
  
  //Move relative
  if (cmd[0] == "MR"){
    startMovement(true, cmd[1].toInt(), cmd[2].toInt());
  }
  
  //Zero location
  if (cmd[0] == "Z"){
    stepperX.setCurrentPosition(0);
    stepperY.setCurrentPosition(0);
  }
  
  //Get Position
  if (cmd[0] == "POS"){
    Serial.print("X-mm: ");
    Serial.println(int(stepperX.currentPosition()/stepsPerMM[0]));
    Serial.print("Y-mm: ");
    Serial.println(int(stepperY.currentPosition()/stepsPerMM[1]));
    Serial.print("");
  }

  //Home X and Y
  if (cmd[0] == "HOME"){
    homeSteppers();
  }

  //Set max Speed
  if (cmd[0] == "SPEED"){
    stepperX.setMaxSpeed(cmd[1].toInt());
    stepperY.setMaxSpeed(cmd[2].toInt());
  }

  //Set steps per MM
  if (cmd[0] == "STEPLEN"){
    stepsPerMM[0] = ( cmd[1].toInt() / 100.0);
    stepsPerMM[1] = ( cmd[2].toInt() / 100.0);
    recalculateDistance();
  }

  //Set X and Y limits
  if (cmd[0] == "LIMITS"){
    input_limitX = cmd[1].toInt() / 100;
    input_limitY = cmd[2].toInt() / 100;
    recalculateDistance();
    hasLimits = true;
  }

  //Set X and Y slop
//  if (cmd[0] == "SLOP"){
//    input_slopX = cmd[1].toInt() / 100;
//    input_slopY = cmd[2].toInt() / 100;
//    hasSlop = true;
//    recalculateDistance();
//  }

  //turn stepper outputs off manually
  if (cmd[0] == "OFF"){
    steppersOff();
  }

  //turn stepper outputs on manually
  if (cmd[0] == "ON"){
    steppersOn();
  }

  //Turn off steppers if we're out of commands
  if (!cmdBuffer.available())
    steppersOff();
}


/////////////////////////////////////////
//MOTOR FUNCTIONS AND SETTINGS
/////////////////////////////////////////

float getNewX(int x) {
  return (x / 100.0)*stepsPerMM[0]; 
}
float getNewY(int y) {
  return (y / 100.0)*stepsPerMM[1]; 
}

//Move to position, only turn motors off if command buffer is empty
void startMovement(bool relative, int x, int y) {
//  long oldX = positions[0];
//  long oldY = positions[1];
//  float slopModifierX;
//  float slopModifierY;
  
  if (relative) {
    positions[0] = (stepperX.currentPosition()+getNewX(x));
    positions[1] = (stepperY.currentPosition()+getNewY(y));
    }
  else {
    positions[0] = getNewX(x);
    positions[1] = getNewY(y);
  }

//  //check Slop
//  if (hasSlop){
//    if (xDir && positions[0] < oldX){
//      //Right to Left
//      stepperX.setCurrentPosition(stepperX.currentPosition()+ (slopX*-1));
//      xDir = false;
//    }
//    if (!xDir && positions[0] > oldX){
//      //Left to Right
//      stepperX.setCurrentPosition(stepperX.currentPosition()+ slopX);
//      xDir = true;
//    }
//    if (yDir && positions[1] < oldY){
//      //Down to Up
//      stepperY.setCurrentPosition(stepperY.currentPosition()+ (slopY*-1));
//      yDir = false;
//    }
//    if (!yDir && positions[1] > oldY){
//      //Up to Down
//      stepperY.setCurrentPosition(stepperY.currentPosition()+ slopY);
//      yDir = true;
//    }
//  }
  
  //Check Limits
  if (hasLimits) {
    positions[0] = max(min(positions[0],limitX),0);
    positions[1] = max(min(positions[1],limitY),0);
  }
  steppersOn();
  steppers.moveTo(positions);
  runSteppersAsync();
}

//Enable Steppers
void steppersOn(){
  digitalWrite(PIN_M1Enable, M_ON);
  digitalWrite(PIN_M2Enable, M_ON);
}

//Disable Steppers
void steppersOff(){
  digitalWrite(PIN_M1Enable, M_OFF);
  digitalWrite(PIN_M2Enable, M_OFF);
}

//Run Step function and keep checking for new commands
void runSteppersAsync(){
  while(steppers.run()){
    requestCommand();
    readSerial();
  }
}

//Home X, Home Y, turn off steppers
void homeSteppers(){
  steppersOn();
  homeX();
  delay(500);
  homeY();
  delay(500);
  positions[0] = 0;
  positions[1] = 0;
  steppers.moveTo(positions);
  runSteppersAsync();
  steppersOff();
}

//Home X
void homeX(){
  stepperX.setMaxSpeed(500);
  stepperX.moveTo(99999);
  while(digitalRead(PIN_INPUT3)){
    stepperX.run();
  }
  stepperX.moveTo(stepperX.currentPosition());
  stepperX.setCurrentPosition(limitX);
  Serial.println(stepperX.currentPosition());
  stepperX.setMaxSpeed(stepperSpeed);
}

//Home Y
void homeY(){
  stepperY.setMaxSpeed(500);
  stepperY.moveTo(-99999);
  while(digitalRead(PIN_INPUT4)){
    stepperY.run();
  }
  stepperY.moveTo(stepperY.currentPosition());
  stepperY.setCurrentPosition(0);
  Serial.println(stepperY.currentPosition());
  stepperY.setMaxSpeed(stepperSpeed);
}

//Recalculate distances
void recalculateDistance(){
  limitX = input_limitX * stepsPerMM[0];
  limitY = input_limitY * stepsPerMM[1];
  slopX = input_slopX * stepsPerMM[0];
  slopY = input_slopY * stepsPerMM[1];
}

/////////////////////////////////////////
//SERIAL FUNCTIONS
/////////////////////////////////////////

void runCommandsFromBuffer(){
  if (cmdBuffer.available()){
    //Serial.println("New command available..");
    parseCommand();
  }
}

void requestCommand() {
  if (cmdBuffer.canEnQueue() && sendRequest){
    Serial.println("CMDREQUEST");
    sendRequest = false;
  }
}

void readSerial() {
  // Read serial input:
  //return if the buffer is full
  if (!cmdBuffer.canEnQueue()){
    return;
  }
  while (Serial.available() > 0) {
    int inChar = Serial.read();

    if (inChar != '\n') { 

      // As long as the incoming byte
      // is not a newline,
      // convert the incoming byte to a char
      // and add it to the string
      serialString += (char)inChar;
    }
    // if you get a newline, print the string,
    // then the string's value as a float:
    else {
      cmdBuffer.enQueue(
        splitValue(serialString,' ',0),
        splitValue(serialString,' ',1),
        splitValue(serialString,' ',2)
      );
      
      //Trying to cut the amount of Serial Commands in file. this slows things down
      //Serial.println("ACK: " + serialString);
      serialString = "";
      sendRequest = true;
    }
  }
}


/////////////////////////////////////////
//UTILITY FUNCTIONS
/////////////////////////////////////////
// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String splitValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
