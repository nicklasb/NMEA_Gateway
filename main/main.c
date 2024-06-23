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


    // INIT NMEA
    ROB_LOGI(log_prefix, "Starting NMEA2000 interface...");
    NMEA2000_Controller_setup();
    char * taskname;
    asprintf(&taskname, "Looking for pilot task");
    robusto_create_task(&look_for_pilot, NULL, taskname , NULL, 0);

    ROB_LOGI(log_prefix, "------------------------------------------");
    ROB_LOGI(log_prefix, "NMEA gateway initiated, awaiting requests.");
    ROB_LOGI(log_prefix, "------------------------------------------");
    while (1)
    {
        NMEA2000_loop();
        robusto_yield();
    }
}