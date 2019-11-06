/*
 * IR_Traffic_Light: control 3 relays for a traffic light with IR_PIN.
 * A visible LED can be connected to STATUS_PIN to provide status.
 *
 * The logic is:
 * Red: Toggle Red light
 * Green: Toggle Green light
 * Blue: Toggle Yellow light (for convenience)
 * Yellow: Also Toggle Yellow light
 * White: Activate all lights
 * On/Off: Toggle all lights (or turn on all if not the case)
 * 
 *
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

int RECV_PIN = 1;
int IR_VCC_PIN = 0;
int RED_OUT = 12; //Should be 12
int GREEN_OUT = 10; //Should be 10
int YELLOW_OUT = 8; //Should be 8

const int REGULAR_MODE = 0;
const int RANDOM_MODE = 1;
const int SIMON_MODE = 2;

const int ONE_SECOND_ISH = 200000;
const int RANDOM_WAIT_TIME = 5*60*ONE_SECOND_ISH;
const int SIMON_ENTRY_TIME = 5*ONE_SECOND_ISH;
const int SIMON_DISPLAY_TIME = ONE_SECOND_ISH;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(IR_VCC_PIN, OUTPUT);
  pinMode(RED_OUT, OUTPUT);
  pinMode(GREEN_OUT, OUTPUT);
  pinMode(YELLOW_OUT, OUTPUT);
  digitalWrite(IR_VCC_PIN, HIGH);
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

// Traffic light abstractions
bool red = true;
bool green = true;
bool yellow = true;

// Mode tracking vars
int mode = REGULAR_MODE;
bool lightsOff = false;

//Timing vars
int counter = 1;

//SIMON_MODE vars
int simonVals[100]; //All current simon values (0-2)
int simonEntry = 0; //Which thing is being displayed / enterred
int simonTop = 1; //Current max score
bool playing = false;

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  if (codeType == NEC) {
    if (results->value == REPEAT) {
      // Ignore NEC repeat value as that's useless.
      return;
    } else {
      int newVal = results->value;
      if (newVal == 16712445) {
        Serial.println("Received NEC: Power on/off");
        lightsOff = !lightsOff;
      } else if (newVal == 16722135) {
        Serial.println("Received NEC: Red Up Arrow");
        switch (mode) {
          case 0:
            mode = 1;
            break;
          case 1:
            mode = 2;
            red = false;
            green = false;
            yellow = false;
            simonEntry = 0;
            simonTop = 1;
            simonVals[0] = random(3);
            break;
          case 2:
            mode = 0;
            break;
        }
        resetCounter();
      } else if (newVal == 16713975) {
        Serial.println("Received NEC: Red Down Arrow");
        switch (mode) {
          case 0:
            mode = 2;
            red = false;
            green = false;
            yellow = false;
            simonEntry = 0;
            simonTop = 1;
            simonVals[0] = random(3);
            break;
          case 1:
            mode = 0;
            break;
          case 2:
            mode = 1;
            break;
        }
        resetCounter();
      }
      switch (mode) {
        case REGULAR_MODE:
          switch (newVal) {
            case 16718565:
              Serial.println("Received NEC: Red");
              red = !red;
              break;
            case 16751205:
              Serial.println("Received NEC: Green");
              green = !green;
              break;
            case 16753245:
              Serial.println("Received NEC: Fake Yellow");
              yellow = !yellow;
              break;
            case 16720605:
              Serial.println("Received NEC: White");
              if (red & green & yellow) {
                red = false;
                green = false;
                yellow = false;
              } else {
                red = true;
                green = true;
                yellow = true;
              }
              break;
            case 16718055:
              Serial.println("Received NEC: Real Yellow");
              yellow = !yellow;
              break;
          }
          break;
        case RANDOM_MODE:
          break;
        case SIMON_MODE:
          if (playing) {
            switch (newVal) {
              case 16718565: //red
                if (simonVals[simonEntry] == 0) {
                  digitalWrite(RED_OUT, HIGH);
                  delay(1000);
                  digitalWrite(RED_OUT, LOW);
                  simonEntry++;
                  resetCounter();
                } else {
                  simonFailure();
                }
                break;
              case 16751205: //green
                if (simonVals[simonEntry] == 1) {
                  digitalWrite(GREEN_OUT, HIGH);
                  delay(1000);
                  digitalWrite(GREEN_OUT, LOW);
                  simonEntry++;
                  resetCounter();
                } else {
                  simonFailure();
                }
                break;
              case 16718055: //yellow
                if (simonVals[simonEntry] == 2) {
                  digitalWrite(YELLOW_OUT, HIGH);
                  delay(1000);
                  digitalWrite(YELLOW_OUT, LOW);
                  simonEntry++;
                  resetCounter();
                } else {
                  simonFailure();
                }
                break;
            }
            if (simonEntry == simonTop) {
              simonTop++;
              simonVals[simonEntry] = random(3);
              simonEntry = 0;
              playing = false;
              resetCounter();
            }
          }
          break;
      }
      
    }
  }
  Serial.println(results->value);
  codeValue = results->value;
  codeLen = results->bits;
}

void resetCounter() {
  counter = 1;
}

void simonFailure() {
  if (!lightsOff) {
    digitalWrite(RED_OUT, HIGH);
    delay(250);
    digitalWrite(RED_OUT, LOW);
    delay(250);
    digitalWrite(RED_OUT, HIGH);
    delay(250);
    digitalWrite(RED_OUT, LOW);
    delay(250);
    digitalWrite(RED_OUT, HIGH);
    delay(250);
    digitalWrite(RED_OUT, LOW);
    delay(250);
    digitalWrite(RED_OUT, HIGH);
    delay(250);
    digitalWrite(RED_OUT, LOW);
    delay(3000);
  }
  simonTop = 1;
  playing = false;
  simonEntry = 0;
  resetCounter();
  simonVals[0] = random(3);
}

void loop() {
  counter++;
  if (irrecv.decode(&results)) {
    storeCode(&results);
    irrecv.resume(); // resume receiver
  }
  if (counter % ONE_SECOND_ISH == 0) {
    Serial.println("Counted to WAIT_TIME");
  }
  switch (mode) {
    case REGULAR_MODE:
      break;
    case RANDOM_MODE:
      if (counter % RANDOM_WAIT_TIME == 0) {
        switch (random(3)) {
          case 0:
            red = !red;
            break;
          case 1:
            green = !green;
            break;
          case 2:
            yellow = !yellow;
            break;
        }
      }
      break;
    case SIMON_MODE:
      if (playing) { //Entry mode
        if (counter % SIMON_ENTRY_TIME == 0) { //ran out of entry time
          simonFailure();
        }
      } else { //Display mode
        if (counter % SIMON_DISPLAY_TIME == 0) {
          if (simonEntry < simonTop) { //Haven't finished displaying
            switch (simonVals[simonEntry]) {
              case 0:
                red = true;
                green = false;
                yellow = false;
                break;
              case 1:
                red = false;
                green = true;
                yellow = false;
                break;
              case 2:
                red = false;
                green = false;
                yellow = true;
                break;
            }
            simonEntry++;
          } else { //Finished displaying
            red = false;
            green = false;
            yellow = false;
            playing = true;
            simonEntry = 0;
            resetCounter();
          }
        }
        if (counter % SIMON_DISPLAY_TIME > (3*SIMON_DISPLAY_TIME/4)) {
          red = false;
          green = false;
          yellow = false;
        }
      }
      break;
  }

  //Leave this alone
  if (red && !lightsOff) {
    digitalWrite(RED_OUT, HIGH);
  } else {
    digitalWrite(RED_OUT, LOW);
  }
  
  if (green & !lightsOff) {
    digitalWrite(GREEN_OUT, HIGH);
  } else {
    digitalWrite(GREEN_OUT, LOW);
  }
  
  if (yellow & !lightsOff) {
    digitalWrite(YELLOW_OUT, HIGH);
  } else {
    digitalWrite(YELLOW_OUT, LOW);
  }
}
