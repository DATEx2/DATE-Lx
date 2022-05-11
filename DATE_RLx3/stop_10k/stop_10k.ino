//intrari
const int int_poz = A3;      // intrare pozitii
const int int_stop = A2;     // intrare stop
const int int_semn_st = A0;  //intrare semnalizare stanga
const int int_semn_dr = A1;  //intrare semnalizare dreapta


//iesiri
const int out_poz_st = 8;
const int out_poz_dr = 9;
const int out_stop = 6;
const int out_st = 7;
const int out_dr = 10;
//altele
unsigned long TurnOffAfterSeconds = 30;
unsigned long lastBlink;
unsigned long lastBlinkState = 0;
unsigned long blinkEveryMs = 5000;
unsigned long blinkForMs = 50;
unsigned long blinkXTimes = 4;
unsigned long sinceLastCommand;
int a = 100;
int franaa ;
int st;
int dr;


//variabile frana
int contFlash;
int numarFlash;
unsigned long delayFrana;
unsigned long previousFrana = 0;
unsigned long intervalFrana = 50;
//const int ledPin =  i2;
int ledState = LOW;
int stateFrana;
int previousstateFrana;


//var semnalizare stanga
int contFlash_st;
int numarFlash_st;
unsigned long delay_st ;
unsigned long previous_st = 0;
unsigned long interval_st_on = 550;
unsigned long interval_st_off = 400;

//const int ledPin =  i2;
int ledState_st = LOW;
int state_st;
int previousstate_st;

//var semnalizare dreapta
int contFlash_dr;
int numarFlash_dr;
unsigned long delay_dr ;
unsigned long previous_dr = 0;
unsigned long interval_dr_on = interval_st_on;
unsigned long interval_dr_off = interval_st_off;

//const int ledPin =  i2;
int ledState_dr = LOW;
int state_dr;
int previousstate_dr;

int stare_poz=0;
int stare_far=0;

//functie frana
void frana() { //Serial.println(stateFrana);
  if (contFlash <= numarFlash) {
    unsigned long currentFrana = millis();
    if (currentFrana - previousFrana >= intervalFrana) {
      //Serial.println(stateFrana);
      // save the last time you blinked the LED
      previousFrana = currentFrana;

      // if the LED is off turn it on and vice-versa:
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      // set the LED with the ledState of the variable:
      digitalWrite(out_stop, ledState);
      contFlash++;
    }
  }
  else {
    ledState = HIGH;
    digitalWrite(out_stop, ledState);
  }

}


//functie semn stanga
void semn_st() { //Serial.println(stateFrana);
  unsigned long current_st = millis();
  if (current_st - previous_st >= (ledState_st == LOW ? interval_st_off : interval_st_on)) {

    previous_st = current_st;
    if (ledState_st == LOW) {
      ledState_st = HIGH;
    } else {
      ledState_st = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(out_st, ledState_st);

  }
}

//functie semn dreapta
void semn_dr() { //Serial.println(stateFrana);
  unsigned long current_dr = millis();
  if (current_dr - previous_dr >= (ledState_dr == LOW ? interval_dr_off : interval_dr_on)) {

    previous_dr = current_dr;
    if (ledState_dr == LOW) {
      ledState_dr = HIGH;
    } else {
      ledState_dr = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(out_dr, ledState_dr);

  }
}

//setari
void setup() {
  Serial.begin(9600);
  sinceLastCommand = millis();
  //frana
  contFlash  = 0;
  numarFlash = 21;
  stateFrana = 0;
  previousstateFrana = 0;
  franaa = 0;
  lastBlink = millis();
  lastBlinkState = 0;
  //st
  state_st = 0;
  previousstate_st = 0;
  st = 0;
  //dr
  state_dr = 0;
  previousstate_dr = 0;
  dr = 0;


  // initialize digital pin LED_BUILTIN as an output.
  //pinMode(ledPin, OUTPUT);

  pinMode(int_semn_dr, INPUT);
  pinMode(int_semn_st, INPUT);
  pinMode(int_stop, INPUT);
  pinMode(int_poz, INPUT);

  pinMode(out_poz_dr, OUTPUT);
  pinMode(out_poz_st, OUTPUT);
  pinMode(out_stop, OUTPUT);
  pinMode(out_st, OUTPUT);
  pinMode(out_dr, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  unsigned long now = millis();
  //frana
  stateFrana = analogRead(int_stop);
  if (stateFrana >= 100 && previousstateFrana <= 100) {
    franaa = 1;
    sinceLastCommand = now; // just now consider the last "active" command the rider did
    previousstateFrana = analogRead(int_stop);
    //digitalWrite(i2, HIGH);

  }
//  if (now >= sinceLastCommand + TurnOffAfterSeconds * 1000) //turn off at 1 minute. Replace it to 20 minutes!
//  {
//    digitalWrite(out_poz_st, LOW);   
//    digitalWrite(out_poz_dr, LOW);
//    digitalWrite(out_st, LOW);
//    digitalWrite(out_dr, LOW);
//    digitalWrite(out_stop, LOW);
//    contFlash = 0;
//    franaa = 0;
//    previousstateFrana = 0;
//    lastBlinkState = 0;
//    //st
//    state_st = 0;
//    previousstate_st = 0;
//    st = 0;
//    //dr
//    state_dr = 0;
//    previousstate_dr = 0;
//    dr = 0;
//    return;
//  }
//  if (blinkLoop(now))
//  {
//    return;
//  }

  if (stateFrana <= 100 && previousstateFrana >= 100) {
    delayFrana = now;
    //franaa=0;
    previousstateFrana = analogRead(int_stop);
  }
  if (stateFrana <= 100) {
    if (now - delayFrana >= 200) {
      franaa = 0;
      digitalWrite(out_stop, LOW);
    }
    contFlash = 0;
  }
  if (franaa == 1) {
    frana();
  }


  //pozitii
  if (analogRead(int_poz) >= 100) {
           if (analogRead(int_semn_st) >= 100) {
                digitalWrite(out_poz_st, ledState_st);}
             else { digitalWrite(out_poz_st, HIGH);}
           if (analogRead(int_semn_dr) >= 100) {
                digitalWrite(out_poz_dr, ledState_dr);}
             else { digitalWrite(out_poz_dr, HIGH);}
        }
      else {
           if (analogRead(int_semn_st) >= 100) {
                digitalWrite(out_poz_st, ledState_st);}
             else {digitalWrite( out_poz_st, LOW);}
           if (analogRead(int_semn_dr) >= 100) {
                digitalWrite(out_poz_dr, ledState_dr);}
             else {digitalWrite(out_poz_dr, LOW);}
        
      }
      

  //st
  if (analogRead(int_semn_st) >= 100) {
    semn_st();
  }
  else {
    digitalWrite(out_st, LOW);
  }

  //dr
  if (analogRead(int_semn_dr) >= 100) {
          semn_dr();
        }
        else {
          digitalWrite(out_dr, LOW);
        }

        
}

bool blinkLoop(unsigned long now) {
  switch(lastBlinkState%2) {
    case 0://blink off
      if (lastBlinkState == 0 && now >= lastBlink + blinkEveryMs
        || lastBlinkState > 0 && now >= lastBlink + blinkForMs) {
        lastBlink = now;
        lastBlinkState++;
        digitalWrite(out_stop, HIGH);
        //digitalWrite(out_poz_st, HIGH);   
        //digitalWrite(out_poz_dr, HIGH);   
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
        int brakeState = completed ? (analogRead(int_stop) < 100 ? LOW : HIGH) : LOW;
        int pozitionSate = completed ? (analogRead(int_poz) < 100 ? LOW : HIGH) : LOW;
        digitalWrite(out_stop, brakeState);
        digitalWrite(out_poz_st, pozitionSate);
        digitalWrite(out_poz_dr, pozitionSate); 
        if (completed) {
          lastBlinkState = 0;
          return false;
        }  
      }
      return true;
      break;
  }
}
