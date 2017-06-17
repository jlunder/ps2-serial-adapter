#include "PSGamepad.h"
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>


//#define PSG_DEBUG

#define PSG_CTRL_BYTE_DELAY 20
#define PSG_READ_DELAY 5000

#define PSC_GET_CONTROL_MAP 0x41
#define PSC_GET_CONTROLS    0x42
#define PSC_SET_CONFIG_MODE 0x43
#define PSC_SET_ANALOG_MODE 0x44
#define PSC_SET_MOTOR_MAP   0x4D
#define PSC_SET_CONTROL_MAP 0x4F


void PSGamepad::begin(uint8_t attentionPin, bool useAnalog, bool usePressure,
    bool useRumble) {
  _attentionPin = attentionPin;
  _rumbleEnabled = useRumble;
  _analogEnabled = useAnalog;
  _pressureEnabled = usePressure;

  _lastReadMillis = millis();

  pinMode(_attentionPin, OUTPUT);
  digitalWrite(_attentionPin, 1);

  SPI.begin();

  // Cause the next poll() to configure the gamepad
  _status = PSCS_DISCONNECTED;
}


void PSGamepad::end() {
  SPI.end();
}


void PSGamepad::poll() {
  poll(false, 0x00);
}


void PSGamepad::poll(bool rumbleMotor0, uint8_t rumbleMotor1) {
  uint32_t deltaMillis = millis() - _lastReadMillis;

  if(deltaMillis < 7) {
    return;
  }

  _lastButtons = _buttons;
  if(_status == PSCS_DISCONNECTED || deltaMillis > 1000) {
    configureGamepad();
  } else {
    readGamepad(rumbleMotor0, rumbleMotor1);
  }

  if(_status == PSCS_CONFIGURING) {
    uint32_t configMillis = millis() - _configureStartMillis;
    if(configMillis > 500) {
      // Something is wrong, trigger a reconfig next poll
#ifdef PSG_DEBUG
      Serial.println("timeout on config");
#endif
      _status = PSCS_DISCONNECTED;
    }
  } else if((_pressureEnabled && _status != PSCS_PRESSURE) ||
      (!_pressureEnabled && _analogEnabled && _status != PSCS_ANALOG) ||
      (!_pressureEnabled && !_analogEnabled && _status != PSCS_DIGITAL)) {
    // Controller is not in the right mode, force it there
#ifdef PSG_DEBUG
    Serial.println("controller in weird state");
#endif
    _status = PSCS_DISCONNECTED;
  }

  _lastReadMillis = millis();
}


void PSGamepad::configureGamepad() {
  // Read state, but with no side effects
  sendCommand(PSC_GET_CONTROLS, NULL, 0, 0x00, NULL, 0);

  bool connected = true;
  connected = connected && setConfigMode(true);
  connected = connected && setAnalogMode(_analogEnabled, true);
  connected = connected && setMotorMap();
  connected = connected && setControlMap(_analogEnabled, _pressureEnabled);
  connected = connected && setConfigMode(false);
  if(connected) {
    _status = PSCS_CONFIGURING;
    _configureStartMillis = millis();
  } else {
#ifdef PSG_DEBUG
    Serial.println("configure didn't all succeed");
#endif
    _status = PSCS_DISCONNECTED;
  }
}


void PSGamepad::readGamepad(bool rumbleMotor0, uint8_t rumbleMotor1) {
  uint8_t rxData[18];
  uint8_t result;

  if(_status == PSCS_CONFIGURING) {
    result = sendCommand(PSC_SET_CONFIG_MODE, NULL, 0, 0x00,
      rxData, sizeof rxData);
  } else {
    uint8_t txData[2] = {
      (uint8_t)(rumbleMotor0 ? 0xFF : 0x00), rumbleMotor1
    };
    result = sendCommand(PSC_GET_CONTROLS, txData, 2, 0x00,
      rxData, sizeof rxData);
  }


  switch(result) {
    case 0x41: case 0x71:
      _status = PSCS_DIGITAL;
      break;
    case 0x43: case 0x73:
      _status = PSCS_ANALOG;
      break;
    case 0xF3:
      if(_status != PSCS_CONFIGURING) {
#ifdef PSG_DEBUG
        Serial.println("unexpected config mode");
#endif
        _status = PSCS_DISCONNECTED;
      }
      break;
    case 0x49: case 0x79:
      _status = PSCS_PRESSURE;
      break;
    default:
#ifdef PSG_DEBUG
      Serial.println("bad result from poll");
#endif
      _status = PSCS_DISCONNECTED;
      break;
  }

  if(_status != PSCS_DISCONNECTED) {
    _buttons = rxData[0] | ((uint16_t)rxData[1] << 8);
    if(_status == PSCS_ANALOG) {
      memcpy(_analogData, rxData + 2, 4);
    }
    if(_status == PSCS_PRESSURE) {
      memcpy(_analogData + 4, rxData + 6, sizeof _analogData - 4);
    }
  }
}


bool PSGamepad::setConfigMode(bool configMode) {
  static uint8_t const enterTxData[] = { 0x01, 0x00 };
  static uint8_t const exitTxData[] = { 0x00, 0x00 };
  uint8_t result = sendCommand(PSC_SET_CONFIG_MODE,
    configMode ? enterTxData : exitTxData,
    sizeof enterTxData, 0x00, NULL, 0);
  return result != 0xFF;
}


bool PSGamepad::setAnalogMode(bool analogMode, bool locked) {
  uint8_t txData[] = {
    (uint8_t)(analogMode ? 0x01 : 0x00),
    (uint8_t)(locked ? 0x03 : 0x00)
  };
  uint8_t result = sendCommand(PSC_SET_ANALOG_MODE,
    txData, sizeof txData, 0x00, NULL, 0);
  return result == 0xF3;
}


bool PSGamepad::setMotorMap() {
  static uint8_t const txData[] = { 0x00, 0x01 };
  uint8_t result = sendCommand(PSC_SET_MOTOR_MAP,
    txData, sizeof txData, 0xFF, NULL, 0);
  return result == 0xF3;
}


bool PSGamepad::setControlMap(bool analog, bool pressure) {
  uint8_t txData[] = { 0x03, 0x00, 0x00, 0x00 };
  if(pressure) {
    txData[0] = 0xFF;
    txData[1] = 0xFF;
    txData[2] = 0x03;
  } else if(analog) {
    txData[0] = 0x3F;
  }

  uint8_t result = sendCommand(PSC_SET_CONTROL_MAP,
    txData, sizeof txData, 0x00, NULL, 0);
  return result == 0xF3;
}


uint8_t PSGamepad::sendCommand(uint8_t command, uint8_t const * txBuf,
    size_t txLength, uint8_t txPad, uint8_t * rxBuf, size_t rxLength) {
  SPI.beginTransaction(SPISettings(250000, LSBFIRST, SPI_MODE3));
  digitalWrite(_attentionPin, 0);

  SPI.transfer(0x01);
  delayMicroseconds(PSG_CTRL_BYTE_DELAY);

  uint8_t response = SPI.transfer(command);
  delayMicroseconds(PSG_CTRL_BYTE_DELAY);

  SPI.transfer(0x00);
  delayMicroseconds(PSG_CTRL_BYTE_DELAY);

#ifdef PSG_DEBUG
  Serial.print("CMD ");
  Serial.print(command, HEX);
  Serial.print('>');
  Serial.print(response, HEX);
  Serial.print(':');
#endif

  if(response != 0xFF) {
    size_t packetSize = (response & 0x0F) << 1;
    size_t i = 0;
    for (; (i < txLength) || (i < packetSize); ++i) {
      uint8_t txByte = ((txBuf != NULL) && (i < txLength) ? txBuf[i] : txPad);
      uint8_t rxByte = SPI.transfer(txByte);
#ifdef PSG_DEBUG
      Serial.print(' ');
      Serial.print(txByte, HEX);
      Serial.print('>');
      Serial.print(rxByte, HEX);
#endif
      if((rxBuf != NULL) && (i < rxLength)) {
        rxBuf[i] = rxByte;
      }
      delayMicroseconds(PSG_CTRL_BYTE_DELAY);
    }
  }

#ifdef PSG_DEBUG
  Serial.println();
#endif
  digitalWrite(_attentionPin, 1);
  SPI.endTransaction();

  delayMicroseconds(PSG_CTRL_BYTE_DELAY);

  return response;
}
