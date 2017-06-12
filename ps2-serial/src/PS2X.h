/******************************************************************
*  Super amazing PS2 controller Arduino Library v1.8
*		details and example sketch: 
*			http://www.billporter.info/?p=240
*
*    Original code by Shutter on Arduino Forums
*
*    Revamped, made into lib by and supporting continued development:
*              Bill Porter
*              www.billporter.info
*
*	 Contributers:
*		Eric Wetzel (thewetzel@gmail.com)
*		Kurt Eckhardt
*
*  Lib version history
*    0.1 made into library, added analog stick support. 
*    0.2 fixed config_gamepad miss-spelling
*        added new functions:
*          NewButtonState();
*          NewButtonState(unsigned int);
*          ButtonPressed(unsigned int);
*          ButtonReleased(unsigned int);
*        removed 'PS' from begining of ever function
*    1.0 found and fixed bug that wasn't configuring controller
*        added ability to define pins
*        added time checking to reconfigure controller if not polled enough
*        Analog sticks and pressures all through 'ps2x.Analog()' function
*        added:
*          enableRumble();
*          enablePressures();
*    1.1  
*        added some debug stuff for end user. Reports if no controller found
*        added auto-increasing sentence delay to see if it helps compatibility.
*    1.2
*        found bad math by Shutter for original clock. Was running at 50kHz, not the required 500kHz. 
*        fixed some of the debug reporting. 
*	1.3 
*	    Changed clock back to 50kHz. CuriousInventor says it's suppose to be 500kHz, but doesn't seem to work for everybody. 
*	1.4
*		Removed redundant functions.
*		Fixed mode check to include two other possible modes the controller could be in.
*       Added debug code enabled by compiler directives. See below to enable debug mode.
*		Added button definitions for shapes as well as colors.
*	1.41
*		Some simple bug fixes
*		Added Keywords.txt file
*	1.5
*		Added proper Guitar Hero compatibility
*		Fixed issue with DEBUG mode, had to send serial at once instead of in bits
*	1.6
*		Changed config_gamepad() call to include rumble and pressures options
*			This was to fix controllers that will only go into config mode once
*			Old methods should still work for backwards compatibility 
*    1.7
*		Integrated Kurt's fixes for the interrupts messing with servo signals
*		Reorganized directory so examples show up in Arduino IDE menu
*    1.8
*		Added Arduino 1.0 compatibility. 
*
*
*
*This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*  
******************************************************************/


// $$$$$$$$$$$$ DEBUG ENABLE SECTION $$$$$$$$$$$$$$$$
// to debug ps2 controller, uncomment these two lines to print out debug to uart

//#define PS2X_DEBUG
//#define PS2X_COM_DEBUG


#ifndef PS2X_lib_h
#define PS2X_lib_h

#include <Arduino.h>
#include <stdint.h>


// Button constants
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
#define PSCS_DIGITAL      1
#define PSCS_ANALOG       2


class PS2X {
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

  uint8_t getStatus() const { return _controllerStatus; }

  void begin(uint8_t attentionPin);
  void begin(uint8_t attentionPin, bool usePressure, bool useRumble);
  void end();
  void readGamepad();
  void readGamepad(bool motor1, uint8_t motor2);

private:
  uint8_t _attentionPin;

  uint8_t _analogData[16];
  uint16_t _lastButtons;
  uint16_t _buttons;
  uint32_t _lastReadMillis;
  uint8_t _readDelay;
  uint8_t _controllerStatus;
  bool _rumbleEnabled;
  bool _pressureEnabled;

  void beginTransaction();
  void transfer(uint8_t const * txBuf, uint8_t * rxBuf, size_t bufLength);
  void endTransaction();
  void sendCommandString(uint8_t const * command, size_t commandLength);
  void reconfigureGamepad();
  void enableRumble();
  bool enableAnalog();
};

#endif
