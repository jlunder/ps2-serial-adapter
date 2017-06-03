#include <Arduino.h>
#include <PS2X_lib.h>

uint32_t const pollMs = 20;
uint32_t lastTick;

PS2X ps2x; // create PS2 Controller Class

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing");
  lastTick = millis();

  int error = ps2x.config_gamepad(13, 11, 10, 12, true, false);
  Serial.print("  config_gamepad returns ");
  Serial.println(error, DEC);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
}

void loop()
{
  uint32_t curTick = millis();
  uint32_t delta = (curTick - lastTick);

  if(delta < pollMs) {
    return;
  }

  if(delta >= pollMs * 2) {
    lastTick = curTick;
  } else {
    lastTick += pollMs;
  }

  digitalWrite(2, 1);
  digitalWrite(3, 1);
  ps2x.read_gamepad();
  digitalWrite(3, 0);
  Serial.write((uint8_t)0x92);
  Serial.write((uint8_t)((ps2x.ButtonDataByte() >> 0) & 0x0F));
  Serial.write((uint8_t)((ps2x.ButtonDataByte() >> 4) & 0x0F));
  Serial.write((uint8_t)((ps2x.ButtonDataByte() >> 8) & 0x0F));
  Serial.write((uint8_t)((ps2x.ButtonDataByte() >> 12) & 0x0F));
  Serial.write((uint8_t)ps2x.Analog(PSS_RX) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSS_RY) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSS_LX) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSS_LY) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_PAD_RIGHT) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_PAD_LEFT) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_PAD_UP) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_PAD_DOWN) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_GREEN) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_RED) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_BLUE) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_PINK) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_L1) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_R1) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_L2) >> 1);
  Serial.write((uint8_t)ps2x.Analog(PSAB_R2) >> 1);
  digitalWrite(2, 0);
}
