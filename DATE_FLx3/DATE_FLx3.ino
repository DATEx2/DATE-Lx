#include <Arduino.h>
#include <OneButton.h>
#include <arduino-timer.h>
#include <EEPROM.h>
#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))
auto autoOffTimerAfterDisplay = timer_create_default();
auto blinkTimer = timer_create_default();


int oHighBeam = 11; // Output high beam
int oLowBeam = 10;  // Output low beam
int oYRL = 9;// Output circle YELLOW light
int oDRL = 8;   // Output circle WHITE light
int oLedRED = A2; // Output RED led button small circle
int oLedGREEN = A5; // Output GREEN led button small circle
int oLedBLUE = 2; // Output BLUE led button small circle
int oCharger = 7; // Output phone charger
int oTurnLeft = 4;//1102; // Output separate fluid left turn signal front side
int oTurnRight = 12;//1104; // Output separate fluid left turn signal front side
int oPositionLeft = 3; // Output separate fluid left turn signal rear side "position light"
int oPositionRight = 13; // Output separate fluid right turn signal rear side "position light"
int oLoudHorn = 6; // Output loud horn

int iSmartButton = A4;  // Input switch rgb button "one touch"
int iTurnLeft = A0; // Input left turn signal
int iTurnRight = A1; // Input right turn signal 
int iOriginalLightOn = A3; // Input original light
int iHorn = A6; // Input horn
int iDisplayOn = A7; // Input "display on"

enum HeadLightState { ManualOff, AutoOff, ManualOn, AutoOn };
HeadLightState headLightState = AutoOff;

int lastDisplayOnState;

int turnSignalIntervalOn = 550; //flash turn signal every ms
int turnSignalIntervalOff = 350; //wait duration between two turn signals
bool prevTurnLeftState = false;
bool prevTurnRightState = false;
//how many flashes we still do (initally 1 = one flash when turns on)
int shouldStillBlinkLeftTurnSignal = 2, shouldStillBlinkRightTurnSignal = 2;
unsigned long ShutDownAfterDisplayInSecondsAfterDisplayTurnsOff = 3; //should be 10s. auto off when the display is turned off
unsigned long TurnOffAfterSecondsEvenWhenDisplayIsStillOn = 1200; //auto off when no manual comands (button or turn signals)
unsigned long sinceLastCommand; //last millis date/time of the last command (when we pushed the one touch button / turn signals)

//short blinks every 5 sec flashes
unsigned long lastBlink; //the millis when we blinked last time (for every 5s flashes)
unsigned long lastBlinkState = 0; //the sate we had last time (for every 5s flashes, so we can flash multiple times)
unsigned long blinkEveryMs = 5000;//every x ms to blink (for every 5s flashes e.g. 5000ms for 5s)
unsigned long blinkForMs = 40; //the duratio of y ms to blink (for every 5s flashes)
unsigned long blinkXTimes = 4; //how many times we should blink (for every 5s flashes)

//store the YRL / DRL state to be saved/restored automatically
enum State { None = 0, AutoRestore = 1, LowBeam = 2, HighBeam = 4, DRL = 8, YRL = 16, DRLFirstTap = 32, Charging = 64, WLED = 128, YRLFlash = 256, Turn3x = 512 };
enum Blink { TurnSignalsOff = 0, TurnLeft = 1, TurnRight = 2, PositionTurnLeftShouldBeOff = 4, PositionTurnRightShouldBeOff = 8, BlinkFlash = 16, BlinkFlashYRL = 32 };
State currentState = (State)(WLED | DRLFirstTap | YRLFlash | Turn3x | AutoRestore);
Blink currentBlink = (Blink)(TurnSignalsOff);
int stateEEPROMAddress = 2;//the address where we save the current state

OneButton button = OneButton(iSmartButton, false, false);

bool blinkOn(void *argument /* optional argument given to in/at/every */) {
   currentBlink = (Blink)(currentBlink & ~PositionTurnLeftShouldBeOff & ~PositionTurnRightShouldBeOff);

  bool shouldTurnLeft = digitalRead(iTurnLeft) == HIGH, shouldTurnRight = digitalRead(iTurnRight) == HIGH;
  if (shouldTurnRight) { shouldStillBlinkLeftTurnSignal = 0; }
  if (shouldTurnLeft) { shouldStillBlinkRightTurnSignal = 0; }

  if ((currentState & Turn3x) != 0) {
    if (!shouldTurnLeft && prevTurnLeftState && !shouldTurnRight) { shouldStillBlinkLeftTurnSignal = 3; }
    if (!shouldTurnRight && prevTurnRightState && !shouldTurnLeft) { shouldStillBlinkRightTurnSignal = 3; }
  }
  if (shouldTurnLeft || shouldStillBlinkLeftTurnSignal > 0 && shouldStillBlinkLeftTurnSignal-- > 0) {
    currentBlink = (Blink)(currentBlink | TurnLeft);
  }

  if (shouldTurnRight || shouldStillBlinkRightTurnSignal > 0 && shouldStillBlinkRightTurnSignal-- > 0) {
    currentBlink = (Blink)(currentBlink | TurnRight);
    
  }
  applyState();
  prevTurnLeftState = shouldTurnLeft; prevTurnRightState = shouldTurnRight;
  return true; // to repeat the action - false to stop
}
bool blinkOff(void *argument /* optional argument given to in/at/every */) {
    if ((currentBlink & TurnLeft) > 0) {
       currentBlink = (Blink)(currentBlink | PositionTurnLeftShouldBeOff);
    }
    if ((currentBlink & TurnRight) > 0) {
       currentBlink = (Blink)(currentBlink | PositionTurnRightShouldBeOff);
    }
    currentBlink = (Blink)(currentBlink & ~TurnLeft & ~TurnRight);
    applyState();
    return true; // to repeat the action - false to stop
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
        currentBlink = (Blink)(currentBlink | BlinkFlash | BlinkFlashYRL);
        applyState();
        return true; 
      }
      return false;
      break;
    case 1://blink on
      if (now > lastBlink + blinkForMs) 
      {
        lastBlink = now;
        lastBlinkState++;
        bool completed = lastBlinkState >= blinkXTimes * 2 - 1;
        currentBlink = (Blink)(currentBlink & ~BlinkFlashYRL);
        if (completed) {
           currentBlink = (Blink)(currentBlink & ~BlinkFlash);
           lastBlinkState = 0;
           applyState();
           return false;
        } else {
          applyState();
        }
      }
      return true;
      break;
  }
}
void saveState(){
  int state = (int)currentState, oldState = readSavedState();
  if (oldState != state) {
    EEPROM.write(stateEEPROMAddress, state % 256);
    EEPROM.write(stateEEPROMAddress + 1, state / 256);
  }
}
State readSavedState() { 
  State readState = (State)(EEPROM.read(stateEEPROMAddress) % 256 | (EEPROM.read(stateEEPROMAddress + 1) * 256));
  return readState;
}
void restoreState() {
  currentState = readSavedState();
  if ((currentState & AutoRestore) != 0) { //auto-save & restore state
    digitalWrite(oLowBeam, (currentState & LowBeam) != 0 ? HIGH : LOW);
    digitalWrite(oHighBeam, (currentState & HighBeam) != 0 ? HIGH : LOW);
    digitalWrite(oDRL, (currentState & DRL) != 0 ? HIGH : LOW);
    digitalWrite(oYRL, (currentState & YRL) != 0 ? HIGH : LOW);
    digitalWrite(oCharger, (currentState & Charging) != 0 ? HIGH : LOW);
    headLightState = (currentState & (LowBeam | HighBeam | DRL | YRL)) != 0 ? AutoOn : AutoOff;
    if ((currentState & Turn3x) != 0) {
       shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
    }
    updateLedLight();
  }    
  sinceLastCommand = millis();
}
bool checkAutoOff() {
  unsigned long now = millis();
  if (now >= sinceLastCommand + TurnOffAfterSecondsEvenWhenDisplayIsStillOn * 1000
    && (headLightState == ManualOn || headLightState == AutoOn)) {    
    saveState();
    longTap();
    lastBlinkState = 0;
    headLightState = AutoOff; //auto off
    return true;
  }
  if ((currentState & YRLFlash) > 0 && blinkLoop(now)) {
    return true;
  }
  return false;
}

void updateLedLight(){
  
  int turnLeft = digitalRead(oTurnLeft) == HIGH, turnRight = digitalRead(oTurnRight) == HIGH;
  if (turnLeft) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW);
  }
  if (turnRight) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW);
  }
  if (turnLeft || turnRight) { }
  else if (digitalRead(oLoudHorn) == HIGH) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW, oYRL, HIGH, oPositionLeft, HIGH, oPositionRight, HIGH);
  } else if (digitalRead(oHighBeam) == HIGH) {
      digitalWriteStateToPins(oLedRED, LOW, oLedGREEN, digitalRead(oCharger), oLedBLUE, HIGH);
  }
  else if (digitalRead(oLowBeam) == HIGH) {
      digitalWriteStateToPins(oLedRED, digitalRead(oCharger), oLedGREEN, HIGH, oLedBLUE, LOW);
  }
  else if (digitalRead(oCharger) == HIGH) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, LOW);
  }
  else {
    if ((currentState & WLED) != 0) {
      digitalWriteStateToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, HIGH);
    } else {
      digitalWriteStateToPins(oLedRED, LOW, oLedGREEN, LOW, oLedBLUE, LOW);
    }
  }
}
void applyState(){
  bool isOn = (headLightState != AutoOff && headLightState != ManualOff);
  if (!isOn && shouldStillBlinkLeftTurnSignal <= 0 && shouldStillBlinkRightTurnSignal <= 0) {
     digitalWritePins(LOW, oHighBeam, oLowBeam, oYRL, oDRL, oCharger, oTurnLeft, oTurnRight, oPositionLeft, oPositionRight, oLoudHorn);
  }
  else {
    bool turnLeft = (currentBlink & TurnLeft) > 0;
    bool turnRight = (currentBlink & TurnRight) > 0;
    bool positionTurnLeftShouldBeOff = (currentBlink & PositionTurnLeftShouldBeOff) > 0;
    bool positionTurnRightShouldBeOff = (currentBlink & PositionTurnRightShouldBeOff) > 0;
    bool positionShouldBeOn = isOn 
                              && ((currentState & DRL) != 0 || (currentState & YRL) != 0 || (currentState & LowBeam) != 0 || (currentState & HighBeam) != 0);
    bool shouldBlink = (currentBlink & BlinkFlash) > 0;
    bool shouldBlinkYRL = (currentBlink & BlinkFlashYRL) > 0;
 
    if (turnLeft || turnRight || positionTurnLeftShouldBeOff || positionTurnRightShouldBeOff) {
       digitalWriteStateToPins(oYRL, turnLeft || turnRight ? HIGH : LOW, 
                               oDRL, (currentState & DRL) != 0 ? HIGH : LOW,
                               oLowBeam, (currentState & LowBeam) != 0 ? HIGH : LOW,
                               oHighBeam, (currentState & HighBeam) != 0 ? HIGH : LOW,
                               oPositionLeft, positionTurnLeftShouldBeOff ? LOW : (turnLeft || positionShouldBeOn ? HIGH : LOW),
                               oTurnLeft, turnLeft ? HIGH : LOW,
                               oPositionRight, positionTurnRightShouldBeOff ? LOW : (turnRight || positionShouldBeOn ? HIGH : LOW), 
                               oTurnRight, turnRight ? HIGH : LOW
                               );
    }
    else if (shouldBlink) {// && positionShouldBeOn) {
     //Serial.printf("isOn %d, currentBlink %d, shouldBlink %d, shouldBlinkYRL %d, turnLeft %d, turnRight %d, positionTurnLeftShouldBeOff %d, positionTurnRightShouldBeOff %d\n", isOn, currentBlink, shouldBlink, shouldBlinkYRL, turnLeft, turnRight, positionTurnLeftShouldBeOff, positionTurnRightShouldBeOff);
  digitalWriteStateToPins(oYRL, shouldBlinkYRL ? HIGH : LOW, 
                              oPositionLeft, shouldBlinkYRL ? HIGH : LOW,
                              oTurnLeft, LOW, oTurnRight, LOW,
                              oPositionRight, shouldBlinkYRL ? HIGH : LOW);
    } else {
      digitalWriteStateToPins(oDRL, (currentState & DRL) != 0 ? HIGH : LOW,
                              oLowBeam, (currentState & LowBeam) != 0 ? HIGH : LOW,
                              oHighBeam, (currentState & HighBeam) != 0 ? HIGH : LOW,
                              oYRL, (currentState & YRL) != 0 ? HIGH : LOW,
                              oPositionLeft, positionShouldBeOn ? HIGH : LOW,
                              oTurnLeft, LOW, oTurnRight, LOW,
                              oPositionRight, positionShouldBeOn ? HIGH : LOW);
    }
  }
  updateLedLight();
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
  int shouldHighBeam = (currentState & LowBeam) != 0 && (currentState & HighBeam) == 0;
  if (shouldHighBeam) {    
    currentState = (State)(currentState | HighBeam | LowBeam);//turn on HighBeam | LowBeam flag
    if ((currentState & Turn3x) != 0) {
      shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 1;
    }
  }
  else {
    if ((currentState & LowBeam) == 0 && (currentState & HighBeam) == 0 && (currentState & DRL) == 0) {
        if ((currentState & DRLFirstTap) != 0) {
          currentState = (State)(currentState & ~YRL);//turn off YRL flag
          currentState = (State)(currentState | DRL);//turn on DRL flag
        }
        else {
          currentState = (State)(currentState & ~HighBeam);//turn off HighBeam flag
          currentState = (State)(currentState | LowBeam);//turn on LowBeam flag

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
      if ((currentState & Turn3x) != 0) {
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
      }
    }
  }
  saveState();
  applyState();
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
        int shouldDRL = digitalRead(oDRL) == HIGH;
        int shouldYRL = digitalRead(oYRL) == HIGH;
        if (shouldYRL) {    
          currentState = (State)(currentState & ~(YRL|DRL));//turn off YRL and DRL flags
          digitalWritePins(LOW, oDRL, oYRL);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 1;
        }
        else if (shouldDRL) {
          currentState = (State)(currentState & ~DRL);//turn off DRL flag
          currentState = (State)(currentState | YRL);//turn on YRL flag
          digitalWritePins(LOW, oDRL);
          digitalWritePins(HIGH, oYRL);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 3;
        }
        else {
          currentState = (State)(currentState & ~YRL);//turn off YRL flag
          currentState = (State)(currentState | DRL);//turn on DRL flag
          digitalWritePins(HIGH, oDRL);
          digitalWritePins(LOW, oYRL);
          shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = 2;
        }
        updateLedLight();
        saveState();
        break;
    }
    case 4: {
        ///4* pressing once on the button when the headlight is off, will turn on only the DRL.
        ///This can be useful during the day when you don't want to consume power with the low beam.
        ///Disabling this feature will skip the DRL step and start the low beam directly
        
        currentState = (State)(currentState ^ DRLFirstTap);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & DRLFirstTap) != 0 ? 2 : 1;
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
        currentState = (State)(currentState ^ YRLFlash);
        saveState();
        shouldStillBlinkLeftTurnSignal = shouldStillBlinkRightTurnSignal = (currentState & YRLFlash) != 0 ? 2 : 1;
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
  sinceLastCommand = millis() + 365 * 24 * 60 * 60 * 1000;
  headLightState = ManualOff; //manual off
  currentState = (State)(currentState & ~(LowBeam | HighBeam | DRL | YRL | Charging));//turn off LowBeam, HighBeam, DRL, YRL, Charging flag
  if ((currentState & Turn3x) != 0) {
    shouldStillBlinkLeftTurnSignal = 2;
    shouldStillBlinkRightTurnSignal = 2;
  }
  //turn off everything//digitalWritePins(LOW, oHighBeam, oLowBeam, oYRL, oDRL, oCharger, oTurnLeft, oTurnRight, oPositionLeft, oPositionRight, oLoudHorn);
  applyState();
}
bool autoTurnOff(void *argument /* optional argument given to in/at/every */) {
  autoOffTimerAfterDisplay.cancel();
  longTap();
  headLightState = AutoOff; //manual off
}
void checkDisplayState(){
  int displayOnState = analogRead(iDisplayOn) > 200;
  if (displayOnState != lastDisplayOnState) {
    lastDisplayOnState = displayOnState;
    if (displayOnState){
      restoreState();
      autoOffTimerAfterDisplay.cancel();
    }
    else {
      if (headLightState == ManualOn || headLightState == AutoOn) {
        saveState();
      }
      if ((currentState & AutoRestore) != 0) {
        autoOffTimerAfterDisplay.cancel();
        autoOffTimerAfterDisplay.in(ShutDownAfterDisplayInSecondsAfterDisplayTurnsOff * 1000, autoTurnOff);
      }
    }
  }
}

void setup() {  
  //Serial.begin(9600); // Input pins
  pinMode(iSmartButton, INPUT); pinMode(iTurnLeft, INPUT); pinMode(iTurnRight, INPUT); pinMode(iHorn, INPUT); pinMode(iDisplayOn, INPUT);
  // Output pins
  pinMode(oHighBeam, OUTPUT);  digitalWrite(oHighBeam,LOW);
  pinMode(oLowBeam, OUTPUT); digitalWrite(oLowBeam,LOW);
  pinMode(oYRL, OUTPUT);  digitalWrite(oYRL,LOW);
  pinMode(oDRL, OUTPUT); digitalWrite(oDRL,LOW);
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
  button.attachDoubleClick(doubleTap); button.attachClick(singleTap); 
  button.attachLongPressStop(longTap); button.attachMultiClick(multipleTap);
  sinceLastCommand = millis();
  int previousSavedState = (int)readSavedState();
  if (previousSavedState == 0 ) {
    saveState();
  } else {
    currentState = (State)(previousSavedState & (WLED | DRLFirstTap | YRLFlash | Turn3x | AutoRestore));
    applyState();
  }

  blinkTimer.every(turnSignalIntervalOn + turnSignalIntervalOff, blinkOff);
  delay(turnSignalIntervalOff);
  blinkTimer.every(turnSignalIntervalOn + turnSignalIntervalOff, blinkOn);
}

void checkHornState() {
  if (analogRead(iHorn) > 200) {
    digitalWrite(oLoudHorn, HIGH);
  }
  else {
    digitalWrite(oLoudHorn, LOW);
  }
}
void loop() {
  checkHornState();
  checkDisplayState();
  button.tick();
  switch (headLightState)
  {
    case AutoOn:
    case ManualOn:
      if (checkAutoOff()) return;
    break;
  }
  blinkTimer.tick();
  autoOffTimerAfterDisplay.tick();
}
