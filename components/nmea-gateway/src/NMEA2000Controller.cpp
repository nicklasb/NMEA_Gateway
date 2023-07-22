#include "../include/NMEA2000Controller.h"

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_log.h>

#include <String>

#include <NMEA2000_esp32.h>
#include <NMEA2000_CAN.h>
#include <N2kDeviceList.h>
#include <N2kMessages.h>
#include <ActisenseReader.h>
#include "../include/RaymarinePilot.h"

#include "../include/espidf_stream.h"
EspIDFStream Serial;

#define CAN_RX_PIN GPIO_NUM_34
#define CAN_TX_PIN GPIO_NUM_32

int SID = 0;

tActisenseReader actisense_reader;
tNMEA2000 *nmea2000;

double speed_through_water = 0;

// Init logging

char *NMEA2000tag = (char *)"NMEA2000";

const IRAM_ATTR unsigned long TransmitMessages[] = {126208UL, 128259UL, 0};
const IRAM_ATTR unsigned long ReceiveMessages[] = {127250L, 65288L, 65379L, 128259UL, 0};

tN2kDeviceList *pN2kDeviceList;

int getDeviceSourceAddress(std::string model)
{
  if (!pN2kDeviceList->ReadResetIsListUpdated())
    return -1;

  ESP_LOGI(NMEA2000tag, "N2kMaxBusDevices: %i", N2kMaxBusDevices);

  for (uint8_t i = 0; i < N2kMaxBusDevices; i++)
  {
    const tNMEA2000::tDevice *device = pN2kDeviceList->FindDeviceBySource(i);
    if (device == 0)
      continue;

    std::string modelVersion = device->GetModelVersion();

    if (modelVersion.find(model) != std::string::npos)
    {
      return device->GetSource();
    }
  }

  return -2;
}

void ToggleLed()
{
  static bool led_state = false;

  gpio_set_level(GPIO_NUM_2, led_state);
  led_state = !led_state;
}

int num_n2k_messages = 0;
void HandleStreamN2kMsg(const tN2kMsg &message)
{

  // ESP_LOGI(NMEA2000tag,"%s", message.Data);
  //  N2kMsg.Print(&Serial);
  // num_n2k_messages++;
  ToggleLed();
  RaymarinePilot::HandleNMEA2000Msg(message);
}

int num_actisense_messages = 0;
void HandleStreamActisenseMsg(const tN2kMsg &message)
{
  // N2kMsg.Print(&Serial);
  num_actisense_messages++;
  ToggleLed();
  nmea2000->SendMsg(message);
}
bool NMEA2000_Controller_setup()
{
  // instantiate the NMEA2000 object
  nmea2000 = new tNMEA2000_esp32(CAN_TX_PIN, CAN_RX_PIN);

  // attachInterrupt(digitalPinToInterrupt(pinVT), handleRemoteInput, RISING);

  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  nmea2000->SetN2kCANSendFrameBufSize(250);
  nmea2000->SetN2kCANReceiveFrameBufSize(250);

  // Set Product information
  nmea2000->SetProductInformation(
      "20210331",             // Manufacturer's Model serial code
      100,                    // Manufacturer's product code
      "NMEA Controller",      // Manufacturer's Model ID
      "1.0.0.0 (2022-09-22)", // Manufacturer's Software version code
      "1.0.0.0 (2022-09-22)"  // Manufacturer's Model version
  );
  // Set device information
  nmea2000->SetDeviceInformation(
      1111, // Unique number. Use e.g. Serial number.
      132,  // 155,  // Device function=Analog to NMEA 2000 Gateway. See codes on
            //  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      25,   // 60,   // Device class=Inter/Intranetwork Device. See codes on
            //  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
      2046  // Just choosen free from code list on
            // http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
  );

#ifdef DEBUG
  //  nmea2000->SetForwardStream(&Serial);            // PC output on due programming port
  //  nmea2000->SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.
  nmea2000->SetForwardOwnMessages(false);
#endif

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode); // N2km_NodeOnly N2km_ListenAndNode
  nmea2000->SetForwardOwnMessages(true);            // do not echo own messages.
  nmea2000->ExtendTransmitMessages(TransmitMessages);
  nmea2000->ExtendReceiveMessages(ReceiveMessages);
  nmea2000->SetMsgHandler(HandleStreamN2kMsg);

#ifdef DEBUG
//  nmea2000->EnableForward(true);
#endif

  nmea2000->Open();

  //  actisense_reader.SetDefaultSource(75);
  //  actisense_reader.SetMsgHandler(HandleStreamActisenseMsg);

  // beep(BEEP_STARTUP);
  ESP_LOGI(NMEA2000tag, "NMEA2000 controller set up,");
  return true;
}

void look_for_pilot()
{

  pN2kDeviceList = new tN2kDeviceList(&NMEA2000);

  unsigned long t = millis();
  while (RaymarinePilot::PilotSourceAddress < 0 && millis() - t < 5000)
  {
    nmea2000->ParseMessages();
    RaymarinePilot::PilotSourceAddress = getDeviceSourceAddress("Raymarine EV-1 Course Computer");
    vTaskDelay(50 / portTICK_RATE_MS);
  }

  if (RaymarinePilot::PilotSourceAddress >= 0)
  {
    ESP_LOGI(NMEA2000tag, "Found EV-1 Pilot: %i", RaymarinePilot::PilotSourceAddress);
  }
  else
  {
    RaymarinePilot::PilotSourceAddress = 204;
    ESP_LOGW(NMEA2000tag, "EV-1 Pilot not found. Defaulting to %i", RaymarinePilot::PilotSourceAddress);
  }
  vTaskDelete(NULL);
}

void NMEA2000_Controller_set_heading(int heading, int change)
{
  tN2kMsg N2kMsg;
  RaymarinePilot::SetEvoPilotCourse(N2kMsg, heading, change);
  nmea2000->SendMsg(N2kMsg);
}

void NMEA2000_Controller_set_mode(enum RaymarinePilotModes ap_mode)
{
  tN2kMsg N2kMsg;
  RaymarinePilot::SetEvoPilotMode(N2kMsg, (RaymarinePilotModes)ap_mode);
  nmea2000->SendMsg(N2kMsg);
}

void NMEA2000_Controller_send_speedThroughWater(double speedThroughWater)
{
  tN2kMsg N2kMsg;
  // Convert to m/s
  SetN2kPGN128259(N2kMsg, SID++, speedThroughWater * 0.514444);

  speed_through_water = speedThroughWater;
  nmea2000->SendMsg(N2kMsg);
}
/**
 * @brief Send rudder angle to the NMEA network
 * @todo Implement
 * @param rudderAngle
 */
/*
void NMEA2000_Controller_send_rudderAngle(double rudderAngle) {
  tN2kMsg N2kMsg;
  SetN2kPGN130577(N2kMsg, N2kDD025_Unavailable,  N2khr_Unavailable, 0, 0, 0, 0, rudderAngle,0,0);
  nmea2000->SendMsg(N2kMsg);
}
*/

enum RaymarinePilotModes get_ap_mode()
{
  return RaymarinePilot::PilotMode;
}

double get_heading_magnetic()
{
  return RaymarinePilot::HeadingMagnetic;
}

double get_heading_true()
{
  return RaymarinePilot::HeadingTrue;
}

double get_target_heading_magnetic()
{
  return RaymarinePilot::TargetHeadingMagnetic;
}

double get_target_heading_true()
{
  return RaymarinePilot::TargetHeadingTrue;
}

double get_speed_through_water()
{
  return speed_through_water;
}

void NMEA2000_loop()
{
  nmea2000->ParseMessages();
  //  actisense_reader.ParseMessages();
}