
#ifndef _RaymarineEnums_H_
#define _RaymarineEnums_H_

  enum RaymarinePilotModes
  {
    PILOT_MODE_STANDBY = 1,
    PILOT_MODE_AUTO = 2,
    PILOT_MODE_WIND = 3,
    PILOT_MODE_TRACK = 4
  };

  enum key_codes
  {
    KEY_PLUS_1 = 0x07f8,
    KEY_PLUS_10 = 0x08f7,
    KEY_MINUS_1 = 0x05fa,
    KEY_MINUS_10 = 0x06f9,
    KEY_MINUS_1_MINUS_10 = 0x21de,
    KEY_PLUS_1_PLUS_10 = 0x22dd,
    KEY_TACK_PORTSIDE = KEY_MINUS_1_MINUS_10,
    KEY_TACK_STARBORD = KEY_PLUS_1_PLUS_10
  };

#endif