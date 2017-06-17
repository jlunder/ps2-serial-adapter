#ifndef PS2X_lib_h
#define PS2X_lib_h

#include <Arduino.h>
#include <stdint.h>


#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000
#define PSB_TRIANGLE    0x1000
#define PSB_CIRCLE      0x2000
#define PSB_CROSS       0x4000
#define PSB_SQUARE      0x8000

#define PSB_GREEN_FRET  0x0200
#define PSB_RED_FRET    0x2000
#define PSB_YELLOW_FRET 0x1000
#define PSB_BLUE_FRET   0x4000
#define PSB_ORANGE_FRET 0x8000
#define PSB_STAR_POWER  0x0100
#define PSB_UP_STRUM    0x0010
#define PSB_DOWN_STRUM  0x0040

// Stick axis indices
#define PSS_RX          0
#define PSS_RY          1
#define PSS_LX          2
#define PSS_LY          3

#define PSS_WHAMMY_BAR  3

// Analog button pressure indices
#define PSAB_PAD_RIGHT   4
#define PSAB_PAD_LEFT    5
#define PSAB_PAD_UP      6
#define PSAB_PAD_DOWN    7
#define PSAB_GREEN       8
#define PSAB_RED         9
#define PSAB_BLUE        10
#define PSAB_PINK        11
#define PSAB_TRIANGLE    8
#define PSAB_CIRCLE      9
#define PSAB_CROSS       10
#define PSAB_SQUARE      11
#define PSAB_L1          12
#define PSAB_R1          13
#define PSAB_L2          14
#define PSAB_R2          15

// Controller statuses
#define PSCS_DISCONNECTED 0
#define PSCS_CONFIGURING  1
#define PSCS_DIGITAL      2
#define PSCS_ANALOG       3
#define PSCS_PRESSURE     4


class PSGamepad {
public:
  uint16_t getButtons() const {
    return _buttons;
  }
  uint8_t getAnalog(uint8_t axis) const {
    return _analogData[axis];
  }

  uint16_t getButtonsPressed() const {
    return ((_lastButtons ^ _buttons) & _buttons);
  }
  uint16_t getButtonsReleased() const {
    return ((_lastButtons ^ _buttons) & _buttons);
  }
  bool isButton(uint16_t button) const {
    return (getButtonsPressed() & button) != 0;
  }
  bool isButtonPressed(uint16_t button) const {
    return (getButtonsPressed() & button) != 0;
  }
  bool isButtonReleased(uint16_t button) const {
    return (getButtonsReleased() & button) != 0;
  }

  uint8_t getStatus() const { return _status; }

  void begin(uint8_t attentionPin, bool useAnalog = true,
    bool usePressure = false, bool useRumble = false);
  void end();
  void poll();
  void poll(bool rumbleMotor0, uint8_t rumbleMotor1);

private:
  uint8_t _attentionPin;
  bool _rumbleEnabled;
  bool _analogEnabled;
  bool _pressureEnabled;
  uint32_t _configureStartMillis;
  uint32_t _lastReadMillis;
  uint16_t _lastButtons;
  uint16_t _buttons;
  uint8_t _analogData[16];
  uint8_t _status;

  void configureGamepad();
  void readGamepad(bool rumbleMotor0, uint8_t rumbleMotor1);
  bool setConfigMode(bool configMode);
  bool setAnalogMode(bool analogMode, bool locked);
  bool setMotorMap();
  bool setControlMap(bool analog, bool pressure);
  uint8_t sendCommand(uint8_t command, uint8_t const * txBuf, size_t txLength,
    uint8_t txPad, uint8_t * rxBuf, size_t rxLength);
};

#endif
