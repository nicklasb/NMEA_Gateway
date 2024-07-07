
#pragma once

#include <stdint.h>

#include <NMEA2000_esp32xx.h>

#include "RaymarineEnums.h"

  class RaymarinePilot
  {
  public:
    static double HeadingMagnetic, HeadingTrue, TargetHeadingMagnetic, TargetHeadingTrue, Variation;

    static bool alarmWaypoint;

    static RaymarinePilotModes PilotMode;
    static int PilotSourceAddress;

    static void SetEvoPilotMode(tN2kMsg &N2kMsg, RaymarinePilotModes mode);
    static void SetEvoPilotWind(tN2kMsg &N2kMsg, double targetWindDirection);
    static void SetEvoPilotCourse(tN2kMsg &N2kMsg, double heading, int change);
    static inline void SetEvoPilotCourse(tN2kMsg &N2kMsg, double heading)
    {
      return SetEvoPilotCourse(N2kMsg, heading, 0);
    }
    static void TurnToWaypointMode(tN2kMsg &N2kMsg);
    static void TurnToWaypoint(tN2kMsg &N2kMsg);
    static void KeyCommand(tN2kMsg &N2kMsg, uint16_t command);

    static bool HandleNMEA2000Msg(const tN2kMsg &N2kMsg);

    static bool ParseN2kPGN65288(const tN2kMsg &N2kMsg, unsigned char &AlarmState, unsigned char &AlarmCode, unsigned char &AlarmGroup);
    static inline bool ParseN2kAlarm(const tN2kMsg &N2kMsg, unsigned char &AlarmState, unsigned char &AlarmCode, unsigned char &AlarmGroup)
    {
      return ParseN2kPGN65288(N2kMsg, AlarmState, AlarmCode, AlarmGroup);
    }

    static bool ParseN2kPGN65379(const tN2kMsg &N2kMsg, unsigned char &Mode, unsigned char &Submode);
    static inline bool ParseN2kPilotState(const tN2kMsg &N2kMsg, unsigned char &Mode, unsigned char &Submode)
    {
      return ParseN2kPGN65379(N2kMsg, Mode, Submode);
    }

    static bool ParseN2kPGN65345(const tN2kMsg &N2kMsg, double &WindAngle, double &RollingAverageWindAngle);
    static inline bool ParseN2kPilotWindAngle(const tN2kMsg &N2kMsg, double &WindAngle, double &RollingAverageWindAngle)
    {
      return ParseN2kPGN65345(N2kMsg, WindAngle, RollingAverageWindAngle);
    }

    static bool ParseN2kPGN65359(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic);
    static inline bool ParseN2kPilotHeading(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic)
    {
      return ParseN2kPGN65360(N2kMsg, HeadingTrue, HeadingMagnetic);
    }
    static bool ParseN2kPGN65360(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic);
    static inline bool ParseN2kPilotLockedHeading(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic)
    {
      return ParseN2kPGN65360(N2kMsg, HeadingTrue, HeadingMagnetic);
    }
  };

