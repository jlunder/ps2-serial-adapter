#include "PS2X.h"
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

//#define PS2X_COM_DEBUG 1

#define CTRL_BYTE_DELAY 20
#define INITIAL_READ_DELAY 5
#define TRANSFER_RATE 250000


namespace {
  static uint8_t const ps2xCmdEnterConfig[] = {0x01,0x43,0x00,0x01,0x00};
  static uint8_t const ps2xCmdSetMode[] = {0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00};
  static uint8_t const ps2xCmdEnableAnalog[] = {0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
  static uint8_t const ps2xCmdExitConfig[] = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
  static uint8_t const ps2xCmdEnableRumble[] = {0x01,0x4D,0x00,0x00,0x01};
  static uint8_t const ps2xCmdReadType[] = {0x01,0x45,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};
  static uint8_t const ps2xCmdReadControls[] = {0x01,0x43,0x00,0x01,0x00};
}


void PS2X::begin(uint8_t attentionPin) {
  return begin(attentionPin, false, false);
}


void PS2X::begin(uint8_t attentionPin, bool usePressure, bool useRumble) {
  _attentionPin = attentionPin;
  _rumbleEnabled = useRumble;
  _pressureEnabled = usePressure;

  pinMode(_attentionPin, OUTPUT);
  digitalWrite(_attentionPin, 1);

  SPI.begin();

  delay(1);

  //try setting mode, increasing delays if need be.
  _readDelay = INITIAL_READ_DELAY;

  for(size_t j = 0; ; ++j)
  {
    //start config run
    sendCommandString(ps2xCmdEnterConfig, sizeof(ps2xCmdEnterConfig));
    delay(1);

    sendCommandString(ps2xCmdSetMode, sizeof(ps2xCmdSetMode));
    delay(10);
    if(useRumble) {
      sendCommandString(ps2xCmdEnableRumble, sizeof(ps2xCmdEnableRumble));
      delay(10);
    }
    if(usePressure) {
      sendCommandString(ps2xCmdEnableAnalog, sizeof(ps2xCmdEnableAnalog));
      delay(10);
    }
    sendCommandString(ps2xCmdExitConfig, sizeof(ps2xCmdExitConfig));
    delay(10);

    readGamepad();
    readGamepad();
    if(_controllerStatus != PSCS_DISCONNECTED) {
      break;
    }

    if(j == 10) {
      break; //exit function with error
    }

    _readDelay += 1; //add 1ms to read_delay
  }
}


void PS2X::end() {
  SPI.end();
}


void PS2X::readGamepad() {
  readGamepad(false, 0x00);
}


void PS2X::readGamepad(bool motor1, uint8_t motor2) {
  uint32_t deltaMillis = millis() - _lastReadMillis;

  if (deltaMillis > 1500) {
    //waited to long
    reconfigureGamepad();
  } else if(deltaMillis < _readDelay) {
    //waited too short
    delay(_readDelay - deltaMillis);
  }

  _lastButtons = _buttons; //store the previous buttons states

  beginTransaction();

#ifdef PS2X_COM_DEBUG
  Serial.print("READ:");
#endif
  uint8_t readCmd[] = {
    0x01, 0x42, 0x00, (motor1 ? (uint8_t)1 : (uint8_t)0), motor2
  };
  uint8_t readResult[sizeof readCmd];
  //Send the command to send button and joystick data;
  transfer(readCmd, readResult, sizeof readResult);
  _buttons = ~(readResult[3] | ((uint16_t)readResult[4] << 8));

  switch(readResult[1]) {
    case 0x73:
      _controllerStatus = PSCS_DIGITAL;
      transfer(NULL, _analogData, 4);
      break;
    case 0x79:
      _controllerStatus = PSCS_ANALOG;
      transfer(NULL, _analogData, 16);
      break;
    default:
      _controllerStatus = PSCS_DISCONNECTED;
      break;
  }
#ifdef PS2X_COM_DEBUG
  Serial.println();
#endif
  endTransaction();

  _lastReadMillis = millis();
}


void PS2X::reconfigureGamepad(){
  sendCommandString(ps2xCmdEnterConfig, sizeof(ps2xCmdEnterConfig));
  sendCommandString(ps2xCmdSetMode, sizeof(ps2xCmdSetMode));
  if (_rumbleEnabled) {
    sendCommandString(ps2xCmdEnableRumble, sizeof(ps2xCmdEnableRumble));
  }
  if (_pressureEnabled) {
    sendCommandString(ps2xCmdEnableAnalog, sizeof(ps2xCmdEnableAnalog));
  }
  sendCommandString(ps2xCmdExitConfig, sizeof(ps2xCmdExitConfig));
}


void PS2X::beginTransaction() {
  SPI.beginTransaction(SPISettings(TRANSFER_RATE, LSBFIRST, SPI_MODE3));
  digitalWrite(_attentionPin, 0);
  delayMicroseconds(CTRL_BYTE_DELAY);
}


void PS2X::transfer(uint8_t const * txBuf, uint8_t * rxBuf,
    size_t bufLength) {
  for (size_t i = 0; i < bufLength; ++i) {
    uint8_t txByte = txBuf ? txBuf[i] : 0;
    uint8_t rxByte = SPI.transfer(txByte);
#ifdef PS2X_COM_DEBUG
    Serial.print(' ');
    Serial.print(txBuf[i], HEX);
    Serial.print('>');
    Serial.print(rxByte, HEX);
#endif
    if(rxBuf != NULL) {
      rxBuf[i] = rxByte;
    }
    delayMicroseconds(CTRL_BYTE_DELAY);
  }
}


void PS2X::endTransaction() {
  digitalWrite(_attentionPin, 1);
  SPI.endTransaction();
}

void PS2X::sendCommandString(uint8_t const * command, size_t commandLength) {
  beginTransaction();

#ifdef PS2X_COM_DEBUG
  Serial.print("CMD:");
  transfer(command, NULL, commandLength);
  Serial.println();
#else
  transfer(command, NULL, commandLength);
#endif

  endTransaction();
  delay(_readDelay);                  //wait a few
}
