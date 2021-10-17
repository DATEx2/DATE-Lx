#include <Arduino.h>
#include <OneButton.h>
#include <arduino-timer.h>
#include <EEPROM.h>
#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))
auto timer = timer_create_default();
auto autoOffTimerAfterDisplay = timer_create_default();

int oHighBeam = 11; // Output high beam
int oLowBeam = 10;  // Output low beam
int oYLR = 9;// Output circle YELLOW light
int oDLR = 8;   // Output circle WHITE light
int oLedRED = A2; // Output RED led button small circle
int oLedGREEN = A5; // Output GREEN led button small circle
int oLedBLUE = 2; // Output BLUE led button small circle
int oCharger = 7; // Output phone charger
int oTurnLeft = 4; // Output separate fluid left turn signal front side
int oTurnRight = 12; // Output separate fluid left turn signal front side
int oPositionLeft = 3; // Output separate fluid left turn signal rear side "position light"
int oPositionRight = 13; // Output separate fluid right turn signal rear side "position light"
int oLoudHorn = 6; // Output loud horn

int iSmartButton = A4;  // Input switch rgb button "one touch"
int iTurnLeft = A1; // Input left turn signal
int iTurnRight = A0; // Input right turn signal 
int iHorn = A6; // Input horn
int iDisplayOn = A7; // Input "display on"

enum HeadLightState { ManualOff, AutoOff, ManualOn, AutoOn };
HeadLightState headLightState = AutoOff;

int lastDisplayOnState;

int turnSignalIntervalOn = 550; //flash turn signal every ms
int turnSignalIntervalOff = 350; //wait duration between two turn signals
unsigned long ShutDownAfterDisplayInSeconds = 1; //auto off when the display is turned off
unsigned long TurnOffAfterSeconds = 90; //auto off when no manual comands (button or turn signals)
unsigned long sinceLastCommand; //last millis date/time of the last command (when we pushed the one touch button / turn signals)

//short blinks every 5 sec flashes
unsigned long lastBlink; //the millis when we blinked last time (for every 5s flashes)
unsigned long lastBlinkState = 0; //the sate we had last time (for every 5s flashes, so we can flash multiple times)
unsigned long blinkEveryMs = 5000;//every x ms to blink (for every 5s flashes e.g. 5000ms for 5s)
unsigned long blinkForMs = 50; //the duratio of y ms to blink (for every 5s flashes)
unsigned long blinkXTimes = 4; //how many times we should blink (for every 5s flashes)
//how many flashes we still do (initally 1 = one flash when turns on)
int shouldStillBlinkLeftTurnSignal;
int shouldStillBlinkRightTurnSignal;
//store the YLR / DLR state to be saved/restored automatically
enum State { None = 0, AutoRestore = 1, LowBeam = 2, HighBeam = 4, DLR = 8, YLR = 16, DLRFirstTap = 32, Charging = 64, WLED = 128, YLRFlash = 256, Turn3x = 512 };

State currentState = (State)(WLED | DLRFirstTap | YLRFlash | Turn3x | AutoRestore);
int stateEEPROMAddress = 1;//the address where we save the current state

OneButton button = OneButton(iSmartButton, false, false);

void setup() {  
  // Input pins
  pinMode(iSmartButton, INPUT);
  pinMode(iTurnLeft, INPUT);
  pinMode(iTurnRight, INPUT);
  pinMode(iHorn, INPUT);
  pinMode(iDisplayOn, INPUT);
  
  // Output pins
  pinMode(oHighBeam, OUTPUT);  digitalWrite(oHighBeam,LOW);
  pinMode(oLowBeam, OUTPUT); digitalWrite(oLowBeam,LOW);
  pinMode(oYLR, OUTPUT);  digitalWrite(oYLR,LOW);
  pinMode(oDLR, OUTPUT); digitalWrite(oDLR,LOW);
  pinMode(oLedRED, OUTPUT);  digitalWrite(oLedRED,LOW);
  pinMode(oLedGREEN, OUTPUT); digitalWrite(oLedGREEN,LOW);
  pinMode(oLedBLUE, OUTPUT);  digitalWrite(oLedBLUE,LOW);
  pinMode(oCharger, OUTPUT);  digitalWrite(oCharger,LOW);
  pinMode(oTurnLeft, OUTPUT);  digitalWrite(oTurnLeft,LOW);
  pinMode(oTurnRight, OUTPUT); digitalWrite(oTurnRight,LOW);
  pinMode(oPositionLeft, OUTPUT); digitalWrite(oPositionLeft,LOW);
  pinMode(oPositionRight, OUTPUT);  digitalWrite(oPositionRight,LOW);
  pinMode(oLoudHorn, OUTPUT);  digitalWrite(oLoudHorn,LOW);

  updateLedLight();
  button.attachDoubleClick(doubleTap);
  button.attachClick(singleTap);
  button.attachLongPressStop(longTap);
  button.attachMultiClick(multipleTap);
  sinceLastCommand = millis();
  if ((int)readState()==0) {
    saveState()
  }
}

void loop() {
  checkDisplayState();
  button.tick();
  switch (headLightState)
  {
    case AutoOn:
    case ManualOn:
    if (checkAutoOff()) return;
    break;
  }
  loopTurnSignals();
  timer.tick();
  autoOffTimerAfterDisplay.tick();
  
}

void digitalWritePins(int state, int p0=0,int p1=0, int p2=0, int p3=0, int p4=0, int p5=0, int p6=0, int p7=0, int p8=0, int p9=0, int p10=0, int p11=0, int p12=0) { 
  int pins[] = {p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12}; 
  for(int i=ARRAY_SIZE(pins);--i>=0;) if (pins[i] != 0) { digitalWrite(pins[i], state); } 
}
void digitalWriteStateToPins(int pins[]) { for(int i=0;i<ARRAY_SIZE(pins);i+=2) if (pins[i] != 0) { digitalWrite(pins[i], pins[i+1]); } }
void digitalWriteStateToPins(int p0=0, int v0=0, int p1=0, int v1=0, int p2=0, int v2=0, int p3=0, int v3=0, int p4=0, int v4=0,
                              int p5=0, int v5=0, int p6=0, int v6=0, int p7=0, int v7=0, int p8=0, int v8=0, int p9=0, int v9=0,
                              int p10=0, int v10=0, int p11=0, int v11=0, int p12=0, int v12=0) {
  int pins[]={p0, v0, p1, v1, p2, v2, p3, v3, p4, v4, p5, v5, p6, v6, p7, v7, p8, v8, p9, v9, p10, v10, p11, v11, p12, v12};
  for(int i=0;i<ARRAY_SIZE(pins);i+=2) if (pins[i] != 0) { digitalWrite(pins[i], pins[i+1]); }
}
                               

bool blinkLoop(unsigned long now) {
  switch(lastBlinkState%2) {
    case 0://blink off
      if (lastBlinkState == 0 && now >= lastBlink + blinkEveryMs
        || lastBlinkState > 0 && now >= lastBlink + blinkForMs) {
        lastBlink = now;
        lastBlinkState++;
        digitalWritePins(HIGH, oPositionLeft, oPositionRight, oTurnLeft, oTurnRight, oYLR);
        return true; 
      }
      return false;
      break;
    case 1://blink off
      if (now > lastBlink + blinkForMs) 
      {
        lastBlink = now;
        lastBlinkState++;
        bool completed = lastBlinkState >= blinkXTimes * 2 - 1;
        int toggleLeftTurnSignal = completed ? (analogRead(iTurnLeft) < 200 ? LOW : HIGH) : LOW;
        int toggleRightTurnSignal = completed ? (analogRead(iTurnRight) < 200 ? LOW : HIGH) : LOW;
        int toggleYLR = completed ? ((currentState & YLR) != 0 || analogRead(iHorn) > 200 ? HIGH : LOW) : LOW;

        digitalWritePins(toggleLeftTurnSignal, oPositionLeft, oTurnLeft);
        digitalWritePins(toggleRightTurnSignal, oPositionRight, oTurnRight);
        digitalWritePins(toggleYLR, oYLR);

        if (completed) {
          lastBlinkState = 0;
          updateLedLight();
          return false;
        }  
      }
      return true;
      break;
  }
}

bool checkAutoOff() {
  unsigned long now = millis();
  if (now >= sinceLastCommand + TurnOffAfterSeconds * 1000) //turn off at 1 minute. Replace it to 20 minutes!
  {
    saveState();
    digitalWritePins(LOW, oHighBeam, oLowBeam, oYLR, oDLR, oCharger, oTurnLeft, oTurnRight, oPositionLeft, oPositionRight, oLoudHorn);
    lastBlinkState = 0;
    headLightState = AutoOff; //auto off
    updateLedLight();
    return true;
  }
  if (blinkLoop(now))
  {
    return true;
  }
  return false;
}

void saveState(){
  int state = (int)currentState;
    ;
  int oldState = readSavedState();
  if (oldState != state) {
    EEPROM.write(stateEEPROMAddress, state);
  }
}
State readSavedState() {
  State readState = (State)EEPROM.read(stateEEPROMAddress);
  return readState;
}
void restoreState() {
  currentState = readSavedState();
  if ((currentState & AutoRestore) != 0) { //auto-save & restore state
    digitalWrite(oLowBeam, (currentState & LowBeam) != 0 ? HIGH : LOW);
    digitalWrite(oHighBeam, (currentState & HighBeam) != 0 ? HIGH : LOW);
    digitalWrite(oDLR, (currentState & DLR) != 0 ? HIGH : LOW);
    digitalWrite(oYLR, (currentState & YLR) != 0 ? HIGH : LOW);
    digitalWrite(oCharger, (currentState & Charging) != 0 ? HIGH : LOW);
    headLightState = (currentState & (LowBeam | HighBeam | DLR | YLR)) != 0 ? AutoOn : AutoOff;
    if ((currentState & Turn3x) != 0) {
       shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
    }
    updateLedLight();
  }    
  sinceLastCommand = millis();
}
void doubleTap() {
  headLightState = ManualOn;
  sinceLastCommand = millis();
  int shouldCharge = digitalRead(oCharger) != HIGH;

  if (shouldCharge) {
    currentState = (State)(currentState | Charging);//turn on Charging flag
    if ((currentState & Turn3x) != 0) {
      shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
    }
    digitalWritePins(HIGH, oCharger);
  }
  else {   
       currentState = (State)(currentState & ~Charging);//turn off Charging flag
    if ((currentState & Turn3x) != 0) {
      shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 1;
    }
    digitalWritePins(LOW, oCharger);
  }
  saveState();
  updateLedLight();
}
void singleTap() {
  headLightState = ManualOn;
  sinceLastCommand = millis();
  int shouldHighBeam = digitalRead(oLowBeam) == HIGH && digitalRead(oHighBeam) == LOW;
  if (shouldHighBeam) {    
    currentState = (State)(currentState | HighBeam | LowBeam);//turn on HighBeam | LowBeam flag
    if ((currentState & Turn3x) != 0) {
      shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
    }
    digitalWritePins(HIGH, oHighBeam, oLowBeam);
    saveState();
  }
  else {
    if (digitalRead(oLowBeam) == LOW && digitalRead(oHighBeam) == LOW && digitalRead(oDLR) == LOW) {
        if ((currentState & DLRFirstTap) != 0) {
          currentState = (State)(currentState & ~YLR);//turn off YLR flag
          currentState = (State)(currentState | DLR);//turn on DLR flag
          digitalWritePins(HIGH, oDLR);
          digitalWritePins(LOW, oYLR);
          saveState();
        }
        else {
          currentState = (State)(currentState & ~HighBeam);//turn off HighBeam flag
          currentState = (State)(currentState | LowBeam);//turn on LowBeam flag
          digitalWritePins(LOW, oHighBeam);
          digitalWritePins(HIGH, oLowBeam);
          saveState();
        }
        if ((currentState & Turn3x) != 0) {
          //blink once the turn signals when you turn it on, for the first time
          shouldStillBlinkLeftTurnSignal = 1;
          shouldStillBlinkRightTurnSignal = 1;
        }
    }
    else {
      currentState = (State)(currentState & ~HighBeam);//turn off HighBeam flag
      currentState = (State)(currentState | LowBeam);//turn on LowBeam flag
      digitalWritePins(LOW, oHighBeam);
      digitalWritePins(HIGH, oLowBeam);
      saveState();
      if ((currentState & Turn3x) != 0) {
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
      }
    }
  }
  updateLedLight();
}
void multipleTap()
{
  sinceLastCommand = millis();
  headLightState = ManualOn; 
  int clicks = button.getNumberClicks();
  switch (clicks) {
    case 3: {
        ///3* Toggle the white circle ON/OFF. Toggles the yellow circle ON/OFF
        ///OFF -> WHITE -> YELLOW -> OFF
        
        int shouldDLR = digitalRead(oDLR) == HIGH;
        int shouldYLR = digitalRead(oYLR) == HIGH;
        if (shouldYLR) {    
          currentState = (State)(currentState & ~(YLR|DLR));//turn off YLR and DLR flags
          digitalWritePins(LOW, oDLR, oYLR);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 1;
        }
        else if (shouldDLR) {
          currentState = (State)(currentState & ~DLR);//turn off DLR flag
          currentState = (State)(currentState | YLR);//turn on YLR flag
          digitalWritePins(LOW, oDLR);
          digitalWritePins(HIGH, oYLR);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 3;
        }
        else {
          currentState = (State)(currentState & ~YLR);//turn off YLR flag
          currentState = (State)(currentState | DLR);//turn on DLR flag
          digitalWritePins(HIGH, oDLR);
          digitalWritePins(LOW, oYLR);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
        }
        updateLedLight();
        saveState();
        break;
    }
    case 4: {
        ///4* pressing once on the button when the headlight is off, will turn on only the DLR.
        ///This can be useful during the day when you don't want to consume power with the low beam.
        ///Disabling this feature will skip the DLR step and start the low beam directly
        
        currentState = (State)(currentState ^ DLRFirstTap);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & DLRFirstTap) != 0 ? 2 : 1;
      }
      break;
    case 5: {
        ///5* The One Touch button is always "ON" as long as the battery key is ON, especially useful
        /// to see where the button is in the dark or e.g. when you suddenly enter a tunnel. 
        /// However, at times, this might not be useful, so you could turn this feature off 
        
        currentState = (State)(currentState ^ WLED);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & WLED) != 0 ? 2 : 1;
      }
      break;
    case 6: {
        ///6* The headlamp flashes the yellow circle every 5 seconds as the car drivers mostly notice 
        ///the changes in their landscape. This is a safety feature. However it could be considered 
        ///annoying to some, and hence it can be easily toggled when needed.
        currentState = (State)(currentState ^ YLRFlash);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & YLRFlash) != 0 ? 2 : 1;
      }
    break;
    case 7: {
        ///7* the turn signals will have an extra 3 blinkers after you stop them. This can be very helpful 
        /// as you turn them on and quickly off but the other participants can still see your intention. 
        ///Many cars have this feature, however, it can be turned off when not desired.
        ///Disabling this also disables the yellow circle/turn signals flash upon turning ON/OFF
        
        currentState = (State)(currentState ^ Turn3x);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & Turn3x) != 0 ? 3 : 1;
    }
    break;
    case 8: {
       ///8* when the AUTO ON/OFF is turned on, the headlight state is automatically saved/restored when
       /// the display turns on/off. This includes all the states
       ///Turning off this feature means every time you need to manually turn on/off the headlight, 
       ///except the 6-9 settings which are still saved
       
       currentState = (State)(currentState ^ AutoRestore);
       saveState();
       shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & AutoRestore) != 0 ? 4 : 1;
     }
     break;
  }
}
void longTap() {
  saveState();
  sinceLastCommand = 0;
  headLightState = ManualOff; //manual off
  currentState = (State)(currentState & ~(LowBeam | HighBeam | DLR | YLR | Charging));//turn off LowBeam, HighBeam, DLR, YLR, Charging flag
  if ((currentState & Turn3x) != 0) {
    shouldStillBlinkLeftTurnSignal = 1;
    shouldStillBlinkRightTurnSignal = 1;
  }
//turn off everything
  digitalWritePins(LOW, oHighBeam, oLowBeam, oYLR, oDLR, oCharger, oTurnLeft, oTurnRight, oPositionLeft, oPositionRight, oLoudHorn);
  updateLedLight();
}
bool autoTurnOff(void *argument /* optional argument given to in/at/every */) {
  shouldStillBlinkRightTurnSignal = 1;
  autoOffTimerAfterDisplay.cancel();
  longTap();
}
void checkDisplayState(){
  int displayOnState = analogRead(iDisplayOn);
  if (displayOnState != lastDisplayOnState) {
    lastDisplayOnState = displayOnState;
    if (displayOnState > 200){
      restoreState();
      autoOffTimerAfterDisplay.cancel();
    }
    else {
      saveState();
      if ((currentState & AutoRestore) != 0) {
        autoOffTimerAfterDisplay.in(ShutDownAfterDisplayInSeconds * 1000, autoTurnOff);
      }
    }
  }
}
void updateLedLight(){
  int turnLeft = digitalRead(oTurnLeft) == HIGH;
  int turnRight = digitalRead(oTurnRight) == HIGH;
  if (turnLeft) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW, oYLR, HIGH, oPositionLeft, HIGH);
  }
  if (turnRight) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW, oYLR, HIGH, oPositionRight, HIGH);
  }
  if (turnLeft || turnRight){
    
  }
  else if (digitalRead(oLoudHorn) == HIGH) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW, oYLR, HIGH, oPositionLeft, HIGH, oPositionRight, HIGH);
  }
  else if (digitalRead(oHighBeam) == HIGH) {
      digitalWriteStateToPins(oLedRED, LOW, oLedGREEN, digitalRead(oCharger), oLedBLUE, HIGH, oYLR, (currentState & YLR) != 0 ? HIGH : LOW, oPositionLeft, HIGH, oPositionRight, HIGH);
  }
  else if (digitalRead(oLowBeam) == HIGH) {
      digitalWriteStateToPins(oLedRED, digitalRead(oCharger), oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, (currentState & YLR) != 0 ? HIGH : LOW, oPositionLeft, HIGH, oPositionRight, HIGH);
  }
  else if (digitalRead(oCharger) == HIGH) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, (currentState & YLR) != 0 ? HIGH : LOW, oPositionLeft, (currentState & YLR) != 0 ? HIGH : LOW, oPositionRight, (currentState & YLR) != 0 ? HIGH : LOW);
  }
  else {
    if ((currentState & WLED) != 0) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, HIGH, oYLR, (currentState & YLR) != 0 ? HIGH : LOW, oPositionLeft, (currentState & YLR) != 0 ? HIGH : LOW, oPositionRight, (currentState & YLR) != 0 ? HIGH : LOW);
    } else {
      digitalWriteStateToPins(oLedRED, LOW, oLedGREEN, LOW, oLedBLUE, LOW, oYLR, (currentState & YLR) != 0 ? HIGH : LOW, oPositionLeft, (currentState & YLR) != 0 ? HIGH : LOW, oPositionRight, (currentState & YLR) != 0 ? HIGH : LOW);
    }
  }
}

int prevTurnLeftState = LOW;
int prevTurnRightState = LOW;
//Executed every 500 miliseconds when the turn signal is activated either left or right so we simulate the position's light as turn signal
bool turnSignalPositionTick(void *argument /* optional argument given to in/at/every */) {
  int stillTurnsLeft = digitalRead(iTurnLeft) == HIGH;
  int stillTurnsRight = digitalRead(iTurnRight) == HIGH;
  int wasTurningLeftLightOn = digitalRead(oTurnLeft) == HIGH;
  int wasTurningRightLightOn = digitalRead(oTurnRight) == HIGH;
 if (stillTurnsLeft || (shouldStillBlinkLeftTurnSignal > 0 && shouldStillBlinkLeftTurnSignal-- > 0)) {
    digitalWritePins(wasTurningLeftLightOn ? LOW : HIGH, oPositionLeft, oTurnLeft);
  }
  if (stillTurnsRight || (shouldStillBlinkRightTurnSignal > 0 && shouldStillBlinkRightTurnSignal-- > 0)) {
    digitalWritePins(wasTurningRightLightOn ? LOW : HIGH, oPositionRight, oTurnRight);
  }
  updateLedLight();
  if (stillTurnsLeft || stillTurnsRight) {
    timer.in( (digitalRead(oPositionRight) == HIGH || digitalRead(oPositionLeft) == HIGH) ? turnSignalIntervalOff : turnSignalIntervalOn, turnSignalPositionTick);
  }
  else {
    //still blink 3 times after you stop the turn signals
    shouldStillBlinkLeftTurnSignal = prevTurnLeftState == HIGH && !stillTurnsRight && (currentState & Turn3x) != 0 ? 3 : shouldStillBlinkLeftTurnSignal;
    shouldStillBlinkRightTurnSignal = prevTurnRightState == HIGH && !stillTurnsLeft && (currentState & Turn3x) != 0 ? 3 : shouldStillBlinkRightTurnSignal;
    timer.cancel();
  }
  return false;// true to repeat the action - false to stop
}
void loopTurnSignals(){

  int shouldTurnLeft = digitalRead(iTurnLeft);
  int shouldTurnRight = digitalRead(iTurnRight);
  if (shouldTurnLeft == HIGH && prevTurnLeftState == LOW || shouldStillBlinkLeftTurnSignal > 0) {
    prevTurnLeftState = shouldTurnLeft;
    digitalWritePins(HIGH, oTurnLeft, oPositionLeft);
    updateLedLight();
    timer.in(turnSignalIntervalOn, turnSignalPositionTick);
  } 
  if (shouldTurnRight == HIGH && prevTurnRightState == LOW || shouldStillBlinkRightTurnSignal > 0) {
    prevTurnRightState = shouldTurnRight;    
    digitalWritePins(HIGH, oTurnRight, oPositionRight);
    updateLedLight();
    timer.in(turnSignalIntervalOn, turnSignalPositionTick);
  } 
  if (shouldStillBlinkLeftTurnSignal <= 0 
   && shouldStillBlinkRightTurnSignal <= 0 
   && (shouldTurnLeft != HIGH && prevTurnLeftState  == HIGH 
   || shouldTurnRight != HIGH && prevTurnRightState == HIGH)) {
    timer.cancel();
    prevTurnLeftState = prevTurnRightState = LOW;
    digitalWriteStateToPins(oTurnLeft, LOW, oPositionLeft, LOW, oTurnRight, LOW, oPositionRight, LOW);
    updateLedLight();
  }
}
