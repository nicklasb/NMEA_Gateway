#include "../include/RaymarinePilot.h"

char *TAG = (char *)"NMEA2000-Raymarine-Pilot";

#include <esp_log.h>

#include "N2kMessages.h"

// TODO: These defines are multiplied, please move into library.
#define SPEED_THROUGH_WATER_PGN 128259UL
#define SPEED_COURSE_OVER_GROUND 129026UL
#define TARGET_HEADING_TRUE 65360UL
#define TARGET_HEADING_MAGNETIC 653601UL // Own differentiator
#define HEADING_MAGNETIC 65359UL

double RaymarinePilot::HeadingTrue = 0;
double RaymarinePilot::HeadingMagnetic = 0;
double RaymarinePilot::TargetHeadingTrue = 0;
double RaymarinePilot::TargetHeadingMagnetic = 0;
double RaymarinePilot::Variation = 0;
int RaymarinePilot::PilotSourceAddress = -1;
RaymarinePilotModes RaymarinePilot::PilotMode = PILOT_MODE_STANDBY;

unsigned int pilotHeadingFilterCount = 0;
unsigned int pilotTargetHeadingFilterCount = 0;

bool RaymarinePilot::alarmWaypoint = false;
message_callback_cb * RaymarinePilot::message_callback = NULL;

void RaymarinePilot::SetMessageCallback(message_callback_cb * callback_cb) {
  RaymarinePilot::message_callback = callback_cb;
}

void RaymarinePilot::CallMessageCallback(int32_t value, uint32_t pgn) {
  if (RaymarinePilot::message_callback) {
    RaymarinePilot::message_callback(value, pgn);
  }
}
// PilotSourceAddress muss aus der tN2kDeviceList ausgelesen werden. Beispiel dazu: DeviceAnalyzer.ino

void RaymarinePilot::SetEvoPilotMode(tN2kMsg &N2kMsg, RaymarinePilotModes mode)
{
  N2kMsg.SetPGN(126208UL);
  N2kMsg.Priority = 3;
  N2kMsg.Destination = PilotSourceAddress;
  N2kMsg.AddByte(1); // Field 1, 1 = Command Message, 2 = Acknowledge Message...
  N2kMsg.AddByte(0x63);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0xf8);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x07);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x04);

  // 0x00 = standby, 0x40 = auto, 0x0100=vane, 0x0180=track
  switch (mode)
  {
  case PILOT_MODE_STANDBY:
    N2kMsg.AddByte(0x00);
    N2kMsg.AddByte(0x00);
    break;
  case PILOT_MODE_AUTO:
    N2kMsg.AddByte(0x40);
    N2kMsg.AddByte(0x00);
    break;
  case PILOT_MODE_WIND:
    N2kMsg.AddByte(0x00);
    N2kMsg.AddByte(0x01);
    break;
  case PILOT_MODE_TRACK:
    N2kMsg.AddByte(0x80);
    N2kMsg.AddByte(0x01);
    break;
  }

  N2kMsg.AddByte(0x05);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);

  /*N2kMsg.Add3ByteInt(126208UL); // Field 2, commanded PGN 3 byte,  LSB is transmitted first
  N2kMsg.AddByte(0xf0 | 0x8 ); // Field 3, 0x8 = Do not change priority and field 4 is nmea reserved
  N2kMsg.AddByte(1); // Field 5, 1 field/value pair
  N2kMsg.AddByte(4); // Field 6, field 4 to be commanded
  N2kMsg.Add4ByteUInt(0); // Field 7, value for first pair =0 for reset distance log.*/
}

void RaymarinePilot::SetEvoPilotCourse(tN2kMsg &N2kMsg, double heading, int change)
{
  double course = heading + change;
  if ((course) >= 360)
  {
    course -= 360;
  }
  else if ((course) < 0)
  {
    course += 360;
  }

  uint16_t courseRadials10000 = (uint16_t)(DegToRad(course) * 10000); //(newCourse * 174.53);

  uint8_t byte0, byte1;
  byte0 = courseRadials10000 & 0xff;
  byte1 = courseRadials10000 >> 8;

  // 01,50,ff,00,f8,03,01,3b,07,03,04,06,00,00

  N2kMsg.SetPGN(126208UL);
  N2kMsg.Priority = 3;
  N2kMsg.Destination = PilotSourceAddress;
  N2kMsg.AddByte(1); // Field 1, 1 = Command Message, 2 = Acknowledge Message...
  N2kMsg.AddByte(0x50);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0xf8);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x07);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x06);
  N2kMsg.AddByte(byte0);
  N2kMsg.AddByte(byte1);
}

void RaymarinePilot::SetEvoPilotWind(tN2kMsg &N2kMsg, double targetWindDirection)
{

  uint16_t targetWind10000 = (uint16_t)(targetWindDirection * 10000);

  uint8_t byte0, byte1;
  byte0 = targetWind10000 & 0xff;
  byte1 = targetWind10000 >> 8;

  // 41,ff,00,f8,03,01,3b,07,03,04,04,00,00

  N2kMsg.SetPGN(126208UL);
  N2kMsg.Priority = 3;
  N2kMsg.Destination = PilotSourceAddress;
  N2kMsg.AddByte(1); // Field 1, 1 = Command Message, 2 = Acknowledge Message...
  N2kMsg.AddByte(0x41);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0xf8);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x07);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(byte0);
  N2kMsg.AddByte(byte1);
}

void RaymarinePilot::TurnToWaypointMode(tN2kMsg &N2kMsg)
{
  //"01,63,ff,00,f8,04,01,3b,07,03,04,04,81,01,05,ff,ff"
  N2kMsg.SetPGN(126208UL);
  N2kMsg.Priority = 3;
  N2kMsg.Destination = PilotSourceAddress;
  N2kMsg.AddByte(0x01); // Field 1, 1 = Command Message, 2 = Acknowledge Message...
  N2kMsg.AddByte(0x63);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0xf8);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x07);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x81);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x05);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
}

void RaymarinePilot::TurnToWaypoint(tN2kMsg &N2kMsg)
{
  // 00,00,ef,01,ff,ff,ff,ff,ff,ff,04,01,3b,07,03,04,04,6c,05,1a,50"

  N2kMsg.SetPGN(126208UL);
  N2kMsg.Priority = 3;
  N2kMsg.Destination = PilotSourceAddress;
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0x00);
  N2kMsg.AddByte(0xef);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x01);
  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x07);
  N2kMsg.AddByte(0x03);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x04);
  N2kMsg.AddByte(0x6c);
  N2kMsg.AddByte(0x05);
  N2kMsg.AddByte(0x1a);
  N2kMsg.AddByte(0x50);
}

void RaymarinePilot::KeyCommand(tN2kMsg &N2kMsg, uint16_t command)
{
  // const key_command = "3b,9f,f0,81,86,21,%s,%s,ff,ff,ff,ff,ff,c1,c2,cd,66,80,d3,42,b1,c8"

  uint8_t commandByte0, commandByte1;
  commandByte0 = command >> 8;
  commandByte1 = command & 0xff;

  N2kMsg.SetPGN(126720UL);
  N2kMsg.Priority = 7;
  N2kMsg.Destination = PilotSourceAddress;

  N2kMsg.AddByte(0x3b);
  N2kMsg.AddByte(0x9f);
  N2kMsg.AddByte(0xf0);
  N2kMsg.AddByte(0x81);
  N2kMsg.AddByte(0x86);
  N2kMsg.AddByte(0x21);
  // N2kMsg.Add2ByteUInt(command);
  N2kMsg.AddByte(commandByte0);
  N2kMsg.AddByte(commandByte1);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xff);
  N2kMsg.AddByte(0xc1);
  N2kMsg.AddByte(0xc2);
  N2kMsg.AddByte(0xcd);
  N2kMsg.AddByte(0x66);
  N2kMsg.AddByte(0x80);
  N2kMsg.AddByte(0xd3);
  N2kMsg.AddByte(0x42);
  N2kMsg.AddByte(0xb1);
  N2kMsg.AddByte(0xc8);
}

bool RaymarinePilot::HandleNMEA2000Msg(const tN2kMsg &N2kMsg)
{
  if (N2kMsg.PGN == 127250L)
  { // Heading
    unsigned char SID;
    tN2kHeadingReference ref;
    double _Deviation = 0;
    double _Variation;
    double _HeadingRad;

    if (ParseN2kHeading(N2kMsg, SID, _HeadingRad, _Deviation, _Variation, ref))
    {
      if (!N2kIsNA(_Variation))
        Variation = _Variation; // Update Variation
      if (!N2kIsNA(_HeadingRad) && !N2kIsNA(Variation))
        _HeadingRad -= Variation;
      HeadingMagnetic = RadToDeg(_HeadingRad);
    }
  }
  else if (N2kMsg.PGN == 65288L)
  { // Alarm
    unsigned char AlarmState;
    unsigned char AlarmCode;
    unsigned char AlarmGroup;

    if (RaymarinePilot::ParseN2kAlarm(N2kMsg, AlarmState, AlarmCode, AlarmGroup))
    {
      ESP_LOGW(TAG, "Alarm Group: %02X, Code: %02X, State: %02X", AlarmGroup, AlarmCode, AlarmState);

      if (AlarmCode == 0x1d && AlarmGroup == 0x01)
      {
        ESP_LOGW(TAG, "Alarm Waypoint");
        alarmWaypoint = true;
      }
    }
  }

  else if (N2kMsg.PGN == 65379L)
  { // Pilot State
    unsigned char Mode;
    unsigned char Submode;

    if (ParseN2kPilotState(N2kMsg, Mode, Submode))
    {
      ESP_LOGI(TAG, "Mode / Submode : %02X %02X", Mode, Submode);

      if (Mode == 0x00 && Submode == 0x00)
      {
        RaymarinePilot::PilotMode = PILOT_MODE_STANDBY;
        ESP_LOGI(TAG, "PILOT_MODE_STANDBY");
      }
      else if (Mode == 0x40 && Submode == 0x00)
      {
        RaymarinePilot::PilotMode = PILOT_MODE_AUTO;
        ESP_LOGI(TAG, "PILOT_MODE_AUTO");
      }
      else if (Mode == 0x00 && Submode == 0x01)
      {
        RaymarinePilot::PilotMode = PILOT_MODE_WIND;
        ESP_LOGI(TAG, "PILOT_MODE_WIND");
      }

      else if (Mode == 0x81 && Submode == 0x01)
      {
        RaymarinePilot::PilotMode = PILOT_MODE_TRACK;
        ESP_LOGI(TAG, "PILOT_MODE_TRACK");
      }

      else if (Mode == 0x80 && Submode == 0x01)
      {
        RaymarinePilot::PilotMode = PILOT_MODE_TRACK;
        ESP_LOGI(TAG, "PILOT_MODE_TRACK 2?");
      }
    }
  }

  else if (N2kMsg.PGN == 65345L)
  { // Pilot Wind Angle
    double WindAngle;
    double RollingAverageWindAngle;

    if (RaymarinePilot::ParseN2kPilotWindAngle(N2kMsg, WindAngle, RollingAverageWindAngle))
    {

      WindAngle = RadToDeg(WindAngle);
      RollingAverageWindAngle = RadToDeg(RollingAverageWindAngle);

      ESP_LOGI(TAG, "Wind angle: %f Rolling average: %f", WindAngle, RollingAverageWindAngle);
    }
  }

  else if (N2kMsg.PGN == 65359L)
  { // Pilot Heading

    pilotHeadingFilterCount++;
    pilotHeadingFilterCount = pilotHeadingFilterCount % 4;

    if (pilotHeadingFilterCount > 0)
    {
      return false;
    }
    double LocalHeadingTrue;
    double LocalHeadingMagnetic;

    if (RaymarinePilot::ParseN2kPGN65359(N2kMsg, LocalHeadingTrue, LocalHeadingMagnetic))
    {

      RaymarinePilot::HeadingTrue = RadToDeg(LocalHeadingTrue);
      RaymarinePilot::HeadingMagnetic = RadToDeg(LocalHeadingMagnetic);

      ESP_LOGI(TAG, "Heading magnetic: %f", RaymarinePilot::HeadingMagnetic);
      RaymarinePilot::CallMessageCallback(RaymarinePilot::HeadingMagnetic, 65359L);

      if (HeadingTrue != N2kDoubleNA)
      {
        ESP_LOGI(TAG, "Heading true: %f", RaymarinePilot::HeadingTrue);
        RaymarinePilot::CallMessageCallback(RaymarinePilot::HeadingTrue, 65360L);
      }
    }
  }
  else if (N2kMsg.PGN == 65360L)
  { // Pilot Target Heading

    pilotTargetHeadingFilterCount++;
    pilotTargetHeadingFilterCount = pilotTargetHeadingFilterCount % 4;

    if (pilotTargetHeadingFilterCount > 0)
    {
      return false;
    }
    double LocalHeadingTrue;
    double LocalHeadingMagnetic;

    if (RaymarinePilot::ParseN2kPGN65360(N2kMsg, LocalHeadingTrue, LocalHeadingMagnetic))
    {

      RaymarinePilot::TargetHeadingTrue = RadToDeg(LocalHeadingTrue);
      RaymarinePilot::TargetHeadingMagnetic = RadToDeg(LocalHeadingMagnetic);

      ESP_LOGI(TAG, "Target Heading magnetic: %f", RaymarinePilot::TargetHeadingMagnetic);
      RaymarinePilot::CallMessageCallback(RaymarinePilot::TargetHeadingMagnetic, TARGET_HEADING_MAGNETIC);

      if (HeadingTrue != N2kDoubleNA)
      {
        ESP_LOGI(TAG, "Target Heading true: %f", RaymarinePilot::TargetHeadingTrue);
        RaymarinePilot::CallMessageCallback(RaymarinePilot::TargetHeadingTrue, TARGET_HEADING_TRUE);
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}

bool RaymarinePilot::ParseN2kPGN65288(const tN2kMsg &N2kMsg, unsigned char &AlarmStatus, unsigned char &AlarmCode, unsigned char &AlarmGroup)
{
  if (N2kMsg.PGN != 65288L)
    return false;

  int Index = 3;

  AlarmStatus = N2kMsg.GetByte(Index);
  AlarmCode = N2kMsg.GetByte(Index);
  AlarmGroup = N2kMsg.GetByte(Index);

  return true;
}

bool RaymarinePilot::ParseN2kPGN65379(const tN2kMsg &N2kMsg, unsigned char &Mode, unsigned char &Submode)
{
  if (N2kMsg.PGN != 65379L)
    return false;

  int Index = 2;

  Mode = N2kMsg.GetByte(Index);
  Submode = N2kMsg.GetByte(Index);

  return true;
}

bool RaymarinePilot::ParseN2kPGN65345(const tN2kMsg &N2kMsg, double &WindAngle, double &RollingAverageWindAngle)
{
  if (N2kMsg.PGN != 65345L)
    return false;

  int Index = 2;

  WindAngle = N2kMsg.Get2ByteDouble(0.0001, Index);
  RollingAverageWindAngle = N2kMsg.Get2ByteDouble(0.0001, Index);

  return true;
}

bool RaymarinePilot::ParseN2kPGN65359(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic)
{
  if (N2kMsg.PGN != 65359L)
    return false;

  int Index = 3;

  HeadingTrue = N2kMsg.Get2ByteUDouble(0.0001, Index);
  Index = 5;
  HeadingMagnetic = N2kMsg.Get2ByteUDouble(0.0001, Index);

  return true;
}

bool RaymarinePilot::ParseN2kPGN65360(const tN2kMsg &N2kMsg, double &HeadingTrue, double &HeadingMagnetic)
{
  if (N2kMsg.PGN != 65360L)
    return false;

  int Index = 3;

  HeadingTrue = N2kMsg.Get2ByteUDouble(0.0001, Index);
  Index = 5;
  HeadingMagnetic = N2kMsg.Get2ByteUDouble(0.0001, Index);

  return true;
}
