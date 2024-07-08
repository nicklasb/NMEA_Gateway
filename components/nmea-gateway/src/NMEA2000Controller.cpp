
#include "../include/NMEA2000Controller.h"



#include <String>

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34

#include <NMEA2000_esp32xx.h>
#include <N2kDeviceList.h>
#include <N2kMessages.h>

#include <robusto_logging.h>
#include <robusto_system.h>
#include <robusto_time.h>
#include <robusto_concurrency.h>

#include "../include/espidf_stream.h"
#include "../include/RaymarinePilot.h"


int SID = 0;
EspIDFStream Serial;

tNMEA2000_esp32xx *nmea2000;
int32_t time_since_last_can_rx = 0;
double speed_through_water = 0;

#define RECOVERY_RETRY_MS 1000  // How long to attempt CAN bus recovery
// Init logging

char *NMEA2000tag = (char *)"NMEA2000";

const IRAM_ATTR unsigned long TransmitMessages[] = {126208UL, 128259UL, 0};
const IRAM_ATTR unsigned long ReceiveMessages[] = {127250L, 65288L, 65379L, 128259UL, 0};

tN2kDeviceList *pN2kDeviceList;

int num_n2k_messages = 0;
int num_sent_n2k_messages = 0;
int total_num_n2k_messages = 0;
int total_num_sent_n2k_messages = 0;

char can_state;


int getDeviceSourceAddress(std::string model)
{
    if (!pN2kDeviceList->ReadResetIsListUpdated())
        return -1;

    ROB_LOGI(NMEA2000tag, "N2kMaxBusDevices: %i", N2kMaxBusDevices);

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
    gpio_set_level(GPIO_NUM_2, !gpio_get_level(GPIO_NUM_2));
}

char * get_nmea_state_string() {
    char *nmea_row = (char *)robusto_malloc(20);
    total_num_sent_n2k_messages+= num_sent_n2k_messages;
    total_num_n2k_messages+= num_n2k_messages + num_sent_n2k_messages;
    
    if (total_num_sent_n2k_messages > 999) {
        total_num_sent_n2k_messages = 999;
    }
    if (total_num_n2k_messages > 99999) {
        total_num_n2k_messages = 99999;
    }
    if (num_n2k_messages > 999) {
        num_n2k_messages = 999;
    }

    sprintf(nmea_row, "N|A%-5d S%-3d R%-3d", total_num_n2k_messages, total_num_sent_n2k_messages, num_n2k_messages);
    num_n2k_messages = 0;
    num_sent_n2k_messages = 0;
    return nmea_row;
}


void HandleStreamN2kMsg(const tN2kMsg &message)
{
    //ROB_LOGI(NMEA2000tag,"%s", message.Data);

    ToggleLed();
    if (!RaymarinePilot::HandleNMEA2000Msg(message))
    {
        /*if (CONFIG_ROB_LOG_MAXIMUM_LEVEL > ROB_LOG_WARN)
        {
            ROB_LOGW(NMEA2000tag,"Unhandled message: %s", message.Data);
        }*/
    };
    num_n2k_messages++;
}
#if 0
void RecoverFromCANBusOff() {
  // This recovery routine first discussed in
  // https://www.esp32.com/viewtopic.php?t=5010 and also implemented in
  // https://github.com/wellenvogel/esp32-nmea2000

  ROB_LOGE(NMEA2000tag,"Recovering CAN bus");
  static bool recovery_in_progress = false;
  static uint32_t recovery_timer = r_millis();
  if (recovery_in_progress && (r_millis() < recovery_timer + RECOVERY_RETRY_MS)     ) {
    return;
  }
  recovery_in_progress = true;
  recovery_timer = 0;
  // Abort transmission
  MODULE_CAN->CMR.B.AT = 1;
  // read SR after write to CMR to settle register changes
  (void)MODULE_CAN->SR.U;

  // Reset error counters
  MODULE_CAN->TXERR.U = 127;
  MODULE_CAN->RXERR.U = 0;
  // Release Reset mode
  MODULE_CAN->MOD.B.RM = 0;
}

void PollCANStatus() {
  // CAN controller registers are SJA1000 compatible.
  // Bus status value 0 indicates bus-on; value 1 indicates bus-off.
  unsigned int bus_status = MODULE_CAN->SR.B.BS;

  switch (bus_status) {
    case 0:
      can_state = 'R';
      break;
    case 1:
      can_state = 'O';
      ROB_LOGW("NMEA2000 controller", "Recover can bus");
      // try to automatically recover
      RecoverFromCANBusOff();
      break;
  }
}

#endif

bool NMEA2000_Controller_setup()
{

    // instantiate the NMEA2000 object
    nmea2000 = new tNMEA2000_esp32xx(GPIO_NUM_32, GPIO_NUM_34);

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
    //nmea2000->SetForwardOwnMessages(false);
#endif
    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
    nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode); // N2km_NodeOnly N2km_ListenAndNode
    nmea2000->SetForwardOwnMessages(false);            // do not echo own messages.
    nmea2000->ExtendTransmitMessages(TransmitMessages);
    nmea2000->ExtendReceiveMessages(ReceiveMessages);
    nmea2000->SetMsgHandler(HandleStreamN2kMsg);

#ifdef DEBUG
  nmea2000->EnableForward(true);
#endif


    nmea2000->Open();

    //  actisense_reader.SetDefaultSource(75);
    //  actisense_reader.SetMsgHandler(HandleStreamActisenseMsg);
    robusto_gpio_set_direction(2, true);
    // beep(BEEP_STARTUP);
    ROB_LOGI(NMEA2000tag, "NMEA2000 controller set up.");
    return true;
}

void look_for_pilot()
{

    pN2kDeviceList = new tN2kDeviceList(nmea2000);


    unsigned long t = r_millis();
    while (RaymarinePilot::PilotSourceAddress < 0 && r_millis() - t < 10000)
    {
        nmea2000->ParseMessages();
        RaymarinePilot::PilotSourceAddress = getDeviceSourceAddress("Raymarine EV-1 Course Computer");
        r_delay(50);
    }

    if (RaymarinePilot::PilotSourceAddress >= 0)
    {
        ROB_LOGI(NMEA2000tag, "Found EV-1 Pilot: %i", RaymarinePilot::PilotSourceAddress);
    }
    else
    {
        RaymarinePilot::PilotSourceAddress = 204;
        ROB_LOGW(NMEA2000tag, "EV-1 Pilot not found. Defaulting to %i", RaymarinePilot::PilotSourceAddress);

    }
    vTaskDelete(NULL);
}

void NMEA2000_Controller_set_heading(double heading, int change)
{
    // TODO: Course or heading? Decide
    tN2kMsg N2kMsg;
    RaymarinePilot::SetEvoPilotCourse(N2kMsg, heading, change);
    nmea2000->SendMsg(N2kMsg);
    num_sent_n2k_messages++;
}

void NMEA2000_Controller_set_mode(enum RaymarinePilotModes ap_mode)
{
    tN2kMsg N2kMsg;
    RaymarinePilot::SetEvoPilotMode(N2kMsg, (RaymarinePilotModes)ap_mode);
    nmea2000->SendMsg(N2kMsg);
    num_sent_n2k_messages++;
}

void NMEA2000_Controller_send_speedThroughWater(double speedThroughWater)
{
    tN2kMsg N2kMsg;
    // Convert to m/s
    SetN2kPGN128259(N2kMsg, SID++, speedThroughWater * 0.514444);

    speed_through_water = speedThroughWater;
    nmea2000->SendMsg(N2kMsg);
    num_sent_n2k_messages++;
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
void set_heading_magnetic(double value)
{
    RaymarinePilot::HeadingMagnetic = value;
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

void set_target_heading_magnetic(double value)
{
    RaymarinePilot::TargetHeadingMagnetic = value;
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
    if (nmea2000)
    {
        robusto_yield();
        nmea2000->ParseMessages();
        
    }

}