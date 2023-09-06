/**
 * @file NMEA2000Controller.h
 * @author Nicklas B (nicklasb@gmail.com)
 * @brief Header file for the C++ wrapper for the Raymarine Pilot Library.
 * @version 0.1
 * @date 2022-09-12
 *
 * @copyright Copyright (c) 2022
 * Original Library source build by Matzam, see: https://github.com/matztam/raymarine-evo-pilot-remote/
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "RaymarineEnums.h"
#include <stdbool.h>


// Force ESP32 as the NMEA2000 CAN selector seems to fail without arduino
#define USE_N2K_CAN 7

    bool NMEA2000_Controller_setup();

    void NMEA2000_Controller_set_heading(int heading, int change);

    void NMEA2000_Controller_set_mode(enum RaymarinePilotModes ap_mode);

    void NMEA2000_Controller_send_speedThroughWater(double speedThroughWater);

    enum RaymarinePilotModes get_ap_mode();

    double get_heading_magnetic();
    double get_heading_true();
    double get_target_heading_magnetic();
    double get_target_heading_true();
    double get_speed_through_water();

    void NMEA2000_loop();
    void look_for_pilot();


#ifdef __cplusplus
} /* extern "C" */
#endif
