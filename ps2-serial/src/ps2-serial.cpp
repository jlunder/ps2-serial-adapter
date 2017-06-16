#include <Arduino.h>
#include <PSGamepad.h>


enum ParserState {
  PS_IDLE,
  PS_MODE_CHANGE,
};

enum Mode {
  M_MANUAL_POLL,
  M_AUTOMATIC_POLL,
};


char const hexDigits[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};


uint32_t const pollMicros = 20000;
uint32_t lastMicros;

PSGamepad psg;

uint32_t lastReadMicros;
uint32_t lastReadDuration;
uint32_t lastLoopDuration;

ParserState parserState = PS_IDLE;
Mode mode = M_MANUAL_POLL;


void pollGamepad();
void executeCommands();
void printGamepadValues();
void resetGamepad();
void setupGamepad();
char * printHexUint32(char * p, uint32_t val);
char * printHexUint16(char * p, uint16_t val);
char * printHexUint8(char * p, uint8_t val);


void setup() {
  Serial.begin(115200);
  Serial.println("RESET");

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  setupGamepad();

  lastMicros = micros();
}


void loop() {
  uint32_t loopMicros;
  uint32_t microsSinceLastPoll;

  do {
    loopMicros = micros();
    microsSinceLastPoll = (loopMicros - lastMicros);
  } while(microsSinceLastPoll < pollMicros);

  if(microsSinceLastPoll >= pollMicros * 2) {
    lastMicros = loopMicros;
  } else {
    lastMicros += pollMicros;
  }

  digitalWrite(2, 1);
  pollGamepad();
  executeCommands();
  digitalWrite(2, 0);

  lastLoopDuration = micros() - loopMicros;
}


void pollGamepad() {
  digitalWrite(3, 1);
  lastReadMicros = micros();
  psg.poll();
  lastReadDuration = micros() - lastReadMicros;
  digitalWrite(3, 0);
}


void executeCommands() {
  digitalWrite(4, 1);
  bool doPrint = false;
  bool doReset = false;

  while(Serial.available()) {
    int command = Serial.read();
    if(parserState == PS_MODE_CHANGE) {
      switch(command) {
        case '0':
          mode = M_MANUAL_POLL;
          break;
        case '1':
          mode = M_AUTOMATIC_POLL;
          break;
      }
      parserState = PS_IDLE;
    } else {
      switch(command) {
        case 'p': case 'P':
          doPrint = true;
          break;
        case 'm': case 'M':
          parserState = PS_MODE_CHANGE;
          mode = M_MANUAL_POLL;
          break;
        case 'r': case 'R':
          doReset = true;
        default:
          break;
      }
    }
  }
  digitalWrite(4, 0);

  digitalWrite(5, 1);
  if(doReset) {
    resetGamepad();
  } else {
    if(doPrint || mode == M_AUTOMATIC_POLL) {
      printGamepadValues();
    }
  }
  digitalWrite(5, 0);
}


void printGamepadValues() {
  char temp[100];
  char * p = temp;

  *(p++) = 'P';
  *(p++) = ' ';
  p = printHexUint32(p, lastReadMicros);
#if 0
  *(p++) = '/';
  p = printHexUint16(p, lastReadDuration);
  *(p++) = ',';
  p = printHexUint16(p, lastLoopDuration);
#endif
  *(p++) = ':';
  p = printHexUint16(p, psg.getButtons());
  *(p++) = '/';
  p = printHexUint8(p, psg.getAnalog(PSS_LX));
  *(p++) = ',';
  p = printHexUint8(p, psg.getAnalog(PSS_LY));
  *(p++) = ';';
  p = printHexUint8(p, psg.getAnalog(PSS_RX));
  *(p++) = ',';
  p = printHexUint8(p, psg.getAnalog(PSS_RY));
  //*(p++) = '/';
  *(p++) = 0;
  Serial.println(temp);
}


void resetGamepad() {
  psg.end();
  setupGamepad();
}


void setupGamepad() {
  psg.begin(10, true, false);
  Serial.print("R ");
  Serial.println(psg.getStatus(), DEC);
}


char * printHexUint32(char * p, uint32_t val) {
  *(p++) = hexDigits[(val >> 28) & 0xF];
  *(p++) = hexDigits[(val >> 24) & 0xF];
  *(p++) = hexDigits[(val >> 20) & 0xF];
  *(p++) = hexDigits[(val >> 16) & 0xF];
  *(p++) = hexDigits[(val >> 12) & 0xF];
  *(p++) = hexDigits[(val >>  8) & 0xF];
  *(p++) = hexDigits[(val >>  4) & 0xF];
  *(p++) = hexDigits[(val >>  0) & 0xF];
  return p;
}

char * printHexUint16(char * p, uint16_t val) {
  *(p++) = hexDigits[(val >> 12) & 0xF];
  *(p++) = hexDigits[(val >>  8) & 0xF];
  *(p++) = hexDigits[(val >>  4) & 0xF];
  *(p++) = hexDigits[(val >>  0) & 0xF];
  return p;
}

char * printHexUint8(char * p, uint8_t val) {
  *(p++) = hexDigits[(val >>  4) & 0xF];
  *(p++) = hexDigits[(val >>  0) & 0xF];
  return p;
}
