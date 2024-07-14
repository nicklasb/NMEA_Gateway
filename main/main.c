#define CAN_RX_IDX CONFIG_CAN_RX_PIN
#define CAN_TX_IDX CONFIG_CAN_TX_PIN

#include <robusto_logging.h>
#include <robusto_init.h>

#ifdef CONFIG_ROBUSTO_UI
#include <screen.h>
#include <nmea_screen.h>
#endif
#include <nmea_service.h>
#include <NMEA2000Controller.h>
#include <robusto_concurrency.h>


char *log_prefix;


void app_main()
{
    #ifndef CONFIG_ROBUSTO_PUBSUB_SERVER
    #error "The pubsub server library must be enabled."
    #endif
    log_prefix = "NMEA_Gateway";
    #ifdef CONFIG_ROBUSTO_UI
    set_cb_nmea_service((ui_cb *)&set_server_stats, (ui_cb *)&set_nmea_stats);
    #endif
    register_nmea_service();

    init_robusto();    

    #ifdef CONFIG_ROBUSTO_UI
    init_screen(log_prefix);
    init_nmea_screen(log_prefix);
    start_nmea_screen();
    #endif
    r_delay(3000);
    // INIT NMEA
    ROB_LOGI(log_prefix, "Setting up NMEA2000 interface...");
    NMEA2000_Controller_setup();
    ROB_LOGI(log_prefix, "Looking for pilot");
    look_for_pilot();
    ROB_LOGI(log_prefix, "Start that the NMEA controller");
    NMEA2000_start();
    ROB_LOGI(log_prefix, "Enable the NMEA message callback");
    register_nmea_service_message_callback();
    while (1)
    {
        robusto_yield();
    }
}