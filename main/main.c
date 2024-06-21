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
  //  register_nmea_service();

    init_robusto();    

    #ifdef CONFIG_ROBUSTO_UI
    init_screen(log_prefix);
    init_nmea_screen(log_prefix);
    start_nmea_screen();
    #endif


    // INIT NMEA
    ROB_LOGI(log_prefix, "Starting NMEA2000 interface...");
    //NMEA2000_Controller_setup();
    char * taskname;
    asprintf(&taskname, "Looking for pilot");
  //  robusto_create_task(&look_for_pilot, NULL, taskname , NULL, 0);
// robusto_peer_t *peer = add_peer_by_mac_address("Consumer", kconfig_mac_to_6_bytes(0x08b61fc0d660), ROBUSTO_MT_ESPNOW);
// robusto_peer_t *peer = add_peer_by_i2c_address("Consumer", 1);

    // robusto_waitfor_byte(&peer->state, PEER_KNOWN_INSECURE, 4000);
    //  TODO: Should I add a send_message_string with 0,0 as default or something? Or even with defines?
    // char *msg = "Hello";
    // send_message_strings(peer, 0,0, (uint8_t*)msg, 6);
    ROB_LOGI(log_prefix, "------------------------------------------");
    ROB_LOGI(log_prefix, "NMEA gateway initiated, awaiting requests.");
    ROB_LOGI(log_prefix, "------------------------------------------");
    while (1)
    {
      //  NMEA2000_loop();
        // ESP_LOGI(TAG, "Server available memory: %i", heap_caps_get_free_size(MALLOC_CAP_8BIT));
        robusto_yield();
    }
}