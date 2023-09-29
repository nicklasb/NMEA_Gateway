#include <robusto_logging.h>
#include <robusto_init.h>
#include <robusto_server_init.h>
#include <robusto_network_init.h>
#include <robusto_misc_init.h>
#include <robusto_concurrency.h>
#ifdef ROBUSTO_UI_MINIMAL
#include <robusto_screen.h>
#endif
#include <nmea_service.h>
#include <NMEA2000Controller.h>

char *log_prefix;

#define CAN_RX_IDX CONFIG_CAN_RX_IDX
#define CAN_TX_IDX CONFIG_CAN_TX_IDX

void app_main()
{
    register_network_service();
    register_server_service();
    register_nmea_service();
    register_misc_service();
    // INIT framework
    init_robusto();

    log_prefix = "NMEA_Gateway";

    // INIT NMEA
    ROB_LOGI(log_prefix, "Starting NMEA2000 interface...");
    NMEA2000_Controller_setup();
// robusto_peer_t *peer = add_peer_by_mac_address("Consumer", kconfig_mac_to_6_bytes(0x08b61fc0d660), ROBUSTO_MT_ESPNOW);
// robusto_peer_t *peer = add_peer_by_i2c_address("Consumer", 1);
#ifdef ROBUSTO_UI_MINIMAL
    robusto_screen_init(log_prefix);
#endif
    // robusto_waitfor_byte(&peer->state, PEER_KNOWN_INSECURE, 4000);
    //  TODO: Should I add a send_message_string with 0,0 as default or something? Or even with defines?
    // char *msg = "Hello";
    // send_message_strings(peer, 0,0, (uint8_t*)msg, 6);
    ROB_LOGI(log_prefix, "------------------------------------------");
    ROB_LOGI(log_prefix, "NMEA gateway initiated, awaiting requests.");
    ROB_LOGI(log_prefix, "------------------------------------------");
    while (1)
    {
        NMEA2000_loop();
        // ESP_LOGI(TAG, "Server available memory: %i", heap_caps_get_free_size(MALLOC_CAP_8BIT));
        robusto_yield();
    }
}