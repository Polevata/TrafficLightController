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

bool red = false;
bool green = false;
bool yellow = false;

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
      switch (newVal) {
        case 16712445:
          Serial.println("Received NEC: On/Off");
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
          red = true;
          green = true;
          yellow = true;
          break;
        case 16718055:
          Serial.println("Received NEC: Real Yellow");
          yellow = !yellow;
          break;
      }
    }
  }
  Serial.println(results->value);
  codeValue = results->value;
  codeLen = results->bits;
}

void loop() {
  if (irrecv.decode(&results)) {
    storeCode(&results);
    irrecv.resume(); // resume receiver
  }
  if (red) {
    digitalWrite(RED_OUT, HIGH);
  } else {
    digitalWrite(RED_OUT, LOW);
  }
  
  if (green) {
    digitalWrite(GREEN_OUT, HIGH);
  } else {
    digitalWrite(GREEN_OUT, LOW);
  }
  
  if (yellow) {
    digitalWrite(YELLOW_OUT, HIGH);
  } else {
    digitalWrite(YELLOW_OUT, LOW);
  }
}
