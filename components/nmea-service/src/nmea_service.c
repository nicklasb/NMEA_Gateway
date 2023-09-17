#include "nmea_service.h"

#include <robusto_init.h>
#include <robusto_logging.h>
#include <robusto_incoming.h> 
#include <robusto_network_service.h>
#include <robusto_message.h>

#include <string.h>

const uint16_t serviceid = 1959;

#define SPEED_THROUGH_WATER_PGN 128259L

void on_incoming(robusto_message_t *message);
void shutdown_nmea_network_service(void) ;


volatile char * nmea_log_prefix;

volatile char _service_name[26] = "NMEA 2000 network service";




volatile network_service_t nmea_service = {
    service_id : serviceid,
    service_name : &_service_name,
    incoming_callback : &on_incoming,
    shutdown_callback: &shutdown_nmea_network_service
};




void on_incoming(robusto_message_t *message) {
    
    if (message->binary_data_length > 0) {
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, message->binary_data, message->binary_data_length);
        if (*(uint32_t *)(message->binary_data) == SPEED_THROUGH_WATER_PGN) {
            ROB_LOGI(nmea_log_prefix, "Got speed from %s: %u knots/100!", message->peer->name, *(uint16_t *)(message->binary_data + sizeof(uint32_t)));
            
        } else {
            ROB_LOGE(nmea_log_prefix, "An unrecognized PGN: %lu", *(uint32_t *)(message->binary_data));
        }
    } else {
        ROB_LOGE(nmea_log_prefix, "Got a message that didn't have any binary data!");
    }
    
}

void shutdown_nmea_service(void) {

    ROB_LOGE(nmea_log_prefix, "nmea service shutdown.");
}


void shutdown_nmea_network_service(void) {

    ROB_LOGE(nmea_log_prefix, "nmea network service shutdown.");
}

void start_nmea_service(void)
{
    if (robusto_register_network_service(&nmea_service) != ROB_OK) {
        ROB_LOGE(nmea_log_prefix, "Failed adding service");
    }
}

void init_nmea_service(char * _log_prefix)
{
    nmea_log_prefix = _log_prefix;
}

void register_nmea_service(void) {
    char * tst = malloc(18);
    strcpy(tst, "NMEA 2000 service");
    register_service(init_nmea_service, start_nmea_service, shutdown_nmea_service, 4, tst);    
}
