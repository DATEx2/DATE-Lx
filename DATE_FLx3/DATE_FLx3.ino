#include <Arduino.h>
#include <OneButton.h>
#include <arduino-timer.h>
#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))
auto timer = timer_create_default();

int oHighBeam = 11; // Iesire faza lunga
int oLowBeam = 10;  // Iesire faza scurta
int oYLR = 9;// Iesire pozitie far galben
int oDLR = 8;   // Iesire pozitie far alb
int oLedRED = A2; // Iesire led butoon rosu
int oLedGREEN = A5; // Iesire led buton verde 
int oLedBLUE = 2; // Iesire led buton albastru
int oCharger = 7; // Iesire oCharger telefon
int oTurnLeft = 4; // Iesire semnal stanga
int oTurnRight = 12; // Iesire semnal dreapta
int oPositionLeft = 3; // Iesire pozitie semnal stanga
int oPositionRight = 13; // Iesire pozitie semnal dreapta
int oLoudHorn = 6; // Iesire

int iSmartButton = A4;  // Intrare switch rgb
int iTurnLeft = A1; // Intrare switch semnalizare Mate
int iTurnRight = A0; // Intrare switch semnalizare Mate
int iHorn = A6; // Intrare claxon / horn
int displayOn = A7; // Intrare

int turnSignalIntervalOn = 550;
int turnSignalIntervalOff = 350;


OneButton button = OneButton(iSmartButton, false, false);

int memories = 0;
int togglePins[] = { 8, 9, 10, 11, 7, 6, 5, 13};

void setup() {
  
  // Input pins
  pinMode(iSmartButton, INPUT);
  pinMode(iTurnLeft, INPUT);
  pinMode(iTurnRight, INPUT);
  pinMode(iHorn, INPUT);
  
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
}
void loop() {
  button.tick();
  loopTurnSignals();
  timer.tick();
}

void digitalWritePins(int state, int p0=0,int p1=0, int p2=0, int p3=0, int p4=0, int p5=0, int p6=0, int p7=0, int p8=0, int p9=0, int p10=0, int p11=0, int p12=0) { 
  int pins[] = {p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12}; 
  for(int i=ARRAY_SIZE(pins);--i>=0;) if (pins[i] != 0) { digitalWrite(pins[i], state); } 
}
void digitalWriteStatesToPins(int pins[]) { for(int i=0;i<ARRAY_SIZE(pins);i+=2) if (pins[i] != 0) { digitalWrite(pins[i], pins[i+1]); } }
void digitalWriteStatesToPins(int p0=0, int v0=0, int p1=0, int v1=0, int p2=0, int v2=0, int p3=0, int v3=0, int p4=0, int v4=0,
                              int p5=0, int v5=0, int p6=0, int v6=0, int p7=0, int v7=0, int p8=0, int v8=0, int p9=0, int v9=0,
                              int p10=0, int v10=0, int p11=0, int v11=0, int p12=0, int v12=0) {
  int pins[]={p0, v0, p1, v1, p2, v2, p3, v3, p4, v4, p5, v5, p6, v6, p7, v7, p8, v8, p9, v9, p10, v10, p11, v11, p12, v12};
  for(int i=0;i<ARRAY_SIZE(pins);i+=2) if (pins[i] != 0) { digitalWrite(pins[i], pins[i+1]); }
}
                               

void doubleTap() {
  int shouldCharge = digitalRead(oCharger);
  if (shouldCharge == HIGH) {    
    digitalWritePins(LOW, oCharger);
  }
  else {
    digitalWritePins(HIGH, oCharger);
  }
  updateLedLight();
}
void singleTap() {
  int shouldHighBeam = digitalRead(oLowBeam) == HIGH && digitalRead(oHighBeam) == LOW;
  if (shouldHighBeam) {    
    digitalWritePins(HIGH, oHighBeam);
    digitalWritePins(HIGH, oLowBeam);
  }
  else {
    if (digitalRead(oLowBeam) == LOW && digitalRead(oHighBeam) == LOW && digitalRead(oDLR) == LOW) {
        digitalWritePins(HIGH, oDLR);
    }
    digitalWritePins(LOW, oHighBeam);
    digitalWritePins(HIGH, oLowBeam);
  }
  updateLedLight();
}
void multipleTap()
{
  int clicks = button.getNumberClicks();
  if (clicks == 3) {
    int shouldDLR = digitalRead(oDLR) == HIGH;
    if (shouldDLR) {    
      digitalWritePins(LOW, oDLR);
    }
    else {
      digitalWritePins(HIGH, oDLR);
    }
    updateLedLight();
  }
}
void longTap() {
  //turn off everything
  digitalWritePins(LOW, oHighBeam, oLowBeam, oYLR, oDLR, oCharger, oTurnLeft, oTurnRight, oPositionLeft, oPositionRight, oLoudHorn);
  updateLedLight();
}
void updateLedLight(){
  if (digitalRead(oTurnLeft) == HIGH) {
      digitalWriteStatesToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, HIGH);
  }
  else if (digitalRead(oTurnRight) == HIGH) {
      digitalWriteStatesToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, HIGH);
  }
  else if (digitalRead(oLoudHorn) == HIGH) {
      digitalWriteStatesToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, HIGH);
  }
  else if (digitalRead(oHighBeam) == HIGH) {
      digitalWriteStatesToPins(oLedRED, LOW, oLedGREEN, digitalRead(oCharger), oLedBLUE, HIGH, oYLR, LOW);
  }
  else if (digitalRead(oLowBeam) == HIGH) {
      digitalWriteStatesToPins(oLedRED, LOW, oLedGREEN, HIGH, oLedBLUE, LOW, oYLR, LOW);
  }
  else if (digitalRead(oCharger) == HIGH) {
      digitalWriteStatesToPins(oLedRED, HIGH, oLedGREEN, LOW, oLedBLUE, LOW, oYLR, LOW);
  }
  else {
      digitalWriteStatesToPins(oLedRED, HIGH, oLedGREEN, HIGH, oLedBLUE, HIGH, oYLR, LOW);
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
 if (stillTurnsLeft) {
    digitalWritePins(wasTurningLeftLightOn ? LOW : HIGH, oPositionLeft, oTurnLeft);
  }
  if (stillTurnsRight) {
    digitalWritePins(wasTurningRightLightOn ? LOW : HIGH, oPositionRight, oTurnRight);
  }
  updateLedLight();
  if (stillTurnsLeft || stillTurnsRight) {
    timer.in( (digitalRead(oPositionRight) == HIGH || digitalRead(oPositionLeft) == HIGH) ? turnSignalIntervalOff : turnSignalIntervalOn, turnSignalPositionTick);
  }
  else {
    timer.cancel();
  }
  return false;// true to repeat the action - false to stop
}
void loopTurnSignals(){

  int shouldTurnLeft = digitalRead(iTurnLeft) == HIGH;
  int shouldTurnRight = digitalRead(iTurnRight) == HIGH;
  if (shouldTurnLeft && prevTurnLeftState == LOW) {
    prevTurnLeftState = HIGH;
    digitalWritePins(HIGH, oTurnLeft, oPositionLeft);
    updateLedLight();
    timer.in(turnSignalIntervalOn, turnSignalPositionTick);
  } 
  if (shouldTurnRight && prevTurnRightState == LOW) {
    prevTurnRightState = HIGH;    
    digitalWritePins(HIGH, oTurnRight, oPositionRight);
    updateLedLight();
    timer.in(turnSignalIntervalOn, turnSignalPositionTick);
  } 
  if (!shouldTurnLeft && prevTurnLeftState  == HIGH 
  || !shouldTurnRight && prevTurnRightState == HIGH){
    timer.cancel();
    prevTurnLeftState = prevTurnRightState = LOW;
    digitalWriteStatesToPins(oTurnLeft, LOW, oPositionLeft, LOW, oTurnRight, LOW, oPositionRight, LOW);
    updateLedLight();
  }
}



//void loop() {
//  if ( digitalRead(iSmartButton) == HIGH && memories < 8) {
//    digitalWrite(togglePins[memories],HIGH);
//    memories++;
//    delay(500);
//  } 
//  if ( digitalRead(iSmartButton) == HIGH && memories >= 8) {
//    for ( int i = 0; i < 8; i++) {
//      digitalWrite(togglePins[i],LOW);
//    }
//    delay(500);
//    memories = 0;
//  }
//  if ( digitalRead(iTurnLeft) == HIGH) {
//    digitalWrite(oTurnLeft,HIGH);
//    digitalWrite(oPositionLeft,HIGH);
//  } else {
//    digitalWrite(oTurnLeft,LOW);
//    digitalWrite(oPositionLeft,LOW);
//  }
//  if ( digitalRead(iTurnRight) == HIGH) {
//    digitalWrite(oTurnRight,HIGH);
//    digitalWrite(oPositionRight,HIGH);
//  } else {
//    digitalWrite(oTurnRight,LOW);
//    digitalWrite(oPositionRight,LOW);
//  }
//}
