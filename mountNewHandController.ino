#include <TimerOne.h>

float SIDEREAL_DAY = 86164.0905;    // Sidereal day in seconds
float MOUNT_WORM = 144;             // Number of teeth for Vixen GP2 worm
float GEAR_RATIO = 2.5;             // Gear ratio, depends on the pulleys 
float MICROSTEPPING = 32;           // Microstepping defined for the motor controlles
float STEPS = 200;                  // Number of steps of the motors 

//Sidereal rate in microseconds
const float siderealRate = (1000000 * SIDEREAL_DAY) / (MOUNT_WORM * GEAR_RATIO * MICROSTEPPING * STEPS);

//Minimun reaction time of the motor (in microsenconds, is 1.9, but we have to put more)
int minMotorTime = 4;

//Define input pins
#define dirRigth 5
#define dirDown 4 
#define dirLeft 6
#define dirUp 3
#define speedx1Pin 8
#define speedx16Pin 9
#define turboPin 7

//Define output pins
#define stepPinAR A5
#define dirPinAR A4
#define stepPinDEC A2
#define dirPinDEC A3
#define microStep0 1 
#define microStep1 0
#define microStep2 2

//Define direction of the motors
#define dirFwd HIGH
#define dirBck LOW

//Status variables
float actualSpeed = 1;
float objectiveSpeed = 1;
bool isSlewingAR = false;
bool isSlewingDec = false;
bool isTracking = false;
float intermediateStep = 0;
int selectedSpeed;

//Input variables
bool inSpeed1x;
bool inSpeed16x;
bool inTurbo;
bool inDirLeft;
bool inDirRight;
bool inDirUp;
bool inDirDown;

void setup() {
  //Microstepping Setup
  pinMode (microStep0, OUTPUT);
  pinMode (microStep1, OUTPUT);
  pinMode (microStep2, OUTPUT);

  //AR and DEC Output Setup
  pinMode (dirPinAR, OUTPUT);
  pinMode (stepPinAR, OUTPUT);
  pinMode (dirPinDEC, OUTPUT);
  pinMode (stepPinDEC, OUTPUT);

  //Input pins configuration
  pinMode(dirUp, INPUT_PULLUP);
  pinMode(dirLeft, INPUT_PULLUP);
  pinMode(dirRigth, INPUT_PULLUP);
  pinMode(dirDown, INPUT_PULLUP);
  pinMode(speedx1Pin, INPUT_PULLUP);  
  pinMode(speedx16Pin, INPUT_PULLUP);  
  pinMode(turboPin, INPUT);  
  digitalWrite(turboPin, HIGH);  
  
  //Set direction and init motors  
  digitalWrite(dirPinAR, dirFwd);
  digitalWrite(dirPinDEC, dirFwd);
  Timer1.initialize(siderealRate);
  objectiveSpeed = 1;
  actualSpeed = 1;
  }

void loop() {
    
  //Set microstepping to 1/32
  digitalWrite(microStep0, HIGH);
  digitalWrite(microStep1, HIGH);
  digitalWrite(microStep2, HIGH);

  //Get inputs
  inSpeed1x = digitalRead(speedx1Pin) == LOW ? true : false;
  inSpeed16x = digitalRead(speedx16Pin) == LOW ? true : false;
  inTurbo = digitalRead(turboPin) == LOW ? true : false;
  inDirLeft = digitalRead(dirLeft) == LOW ? true : false;
  inDirRight = digitalRead(dirRigth) == LOW ? true : false;
  inDirUp = digitalRead(dirUp) == LOW ? true : false;
  inDirDown = digitalRead(dirDown) == LOW ? true : false;

  //Set timer
  if (!isSlewingAR && !isSlewingDec)
  {
    Timer1.setPeriod(siderealRate); 
    Timer1.attachInterrupt(move_tracking);
  }

  //Check speed buttons
  if (inSpeed1x)
  {
    selectedSpeed = 1;
  }
  else if (inSpeed16x)
  {
    selectedSpeed = 16;
  }
  else
  {
    selectedSpeed = 8;
  }

  if (inTurbo)
  {
    selectedSpeed = selectedSpeed * 50;
  }

  //Check if we have pushed any button
  if (inDirUp)
  {
    isTracking = true;  
    isSlewingAR = false; 
    isSlewingDec = true;
    objectiveSpeed = selectedSpeed;
  }  
  else if (inDirDown)
  {
    isTracking = true;  
    isSlewingAR = false; 
    isSlewingDec = true;
    objectiveSpeed = selectedSpeed;
    digitalWrite(dirPinDEC, dirBck);    
  }
  else if (inDirRight)
  { 
    isTracking = false;
    isSlewingAR = true; 
    isSlewingDec = false;
    objectiveSpeed = selectedSpeed;
  }
  else if (inDirLeft)
  { 
    isTracking = false;
    isSlewingAR = true; 
    isSlewingDec = false;
    objectiveSpeed = selectedSpeed;
    digitalWrite(dirPinAR, dirBck);    
  }
  else
  {  
    isTracking = true; 
    objectiveSpeed = 1;
  }

  //This section controls the acceleration/movement of the motor
  //We treat x1 speed in RA as "special"
  if (selectedSpeed == 1 && inDirRight)
  {    
    objectiveSpeed = 2;
    actualSpeed = 2;   
    Timer1.setPeriod(siderealRate/actualSpeed);
  } 
  else if (selectedSpeed == 1 && inDirLeft)
  {
    objectiveSpeed = 0;
    actualSpeed = 0;   
    Timer1.detachInterrupt();
  } 
  else if (selectedSpeed == 1 && (inDirUp || inDirDown))
  {    
    actualSpeed = 1;   
    Timer1.setPeriod(siderealRate/actualSpeed);
  } 
  else
  {
    //We are not a 1x speed, we work normally
    if (objectiveSpeed <= 1 && actualSpeed <= 1)
    {
      //We are moving at sidereal rate, and we have not pushed any button
      isSlewingAR = false;
      isSlewingDec = false;
      actualSpeed = 1;
      objectiveSpeed = 1;
      digitalWrite(dirPinAR, dirFwd);
      digitalWrite(dirPinDEC, dirFwd);
    }
    else
    {
      if (selectedSpeed >= 50)
      {
        //In this case we have to accelerate or deccelare
        if (objectiveSpeed > actualSpeed)
        {
          //We have to accelerate as we have pushed a button
          actualSpeed = actualSpeed + 0.075;
          Timer1.setPeriod(siderealRate/actualSpeed);
        }
        else if (objectiveSpeed < actualSpeed)
        {
          //We have do deccelerate as we have just released a button
          actualSpeed = actualSpeed - 0.075;   
          Timer1.setPeriod(siderealRate/actualSpeed);
        }
      }
      else
      {
        //No acceleration needed at low speed
        actualSpeed = objectiveSpeed;
        Timer1.setPeriod(siderealRate/actualSpeed);                 
      }    
    }
  }  
}

void move_tracking()
{
  //Move AR
  if (isSlewingAR || (isTracking && !isSlewingDec) || (isSlewingDec && intermediateStep >= actualSpeed))
  {
    digitalWrite(stepPinAR, HIGH);  
    delayMicroseconds(minMotorTime);
    digitalWrite(stepPinAR, LOW);  

    intermediateStep = 0;
  }
  
  //Mode DEC
  if (isSlewingDec)
  {
    digitalWrite(stepPinDEC, HIGH);  
    delayMicroseconds(minMotorTime);
    digitalWrite(stepPinDEC, LOW);  

    intermediateStep = intermediateStep + 1;
  }
}
