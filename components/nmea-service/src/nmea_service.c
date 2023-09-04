#include "nmea_service.h"

#include <robusto_init.h>
#include <robusto_logging.h>
#include <robusto_incoming.h> 
#include <robusto_network_service.h>
#include <robusto_message.h>
#include <robusto_queue.h>
#include <string.h>

static robusto_peer_t *r_peer;
const uint16_t serviceid = 1959;

void register_nmea_service()__attribute__((constructor));

void on_incoming(robusto_message_t *message);
void shutdown_nmea_network_service(void);


char * nmea_log_prefix;

char _service_name[26] = "NMEA 2000 network service";

network_service_t nmea_service = {
    service_id : serviceid,
    service_name : &_service_name,
    incoming_callback : &on_incoming,
    shutdown_callback: &shutdown_nmea_network_service
};

void on_incoming(robusto_message_t *message) {
    
    if (message->string_count == 1) {
        if (strcmp(message->strings[0], "Hi there!") == 0) {
            ROB_LOGW(nmea_log_prefix, "Got a message from the %s client through %s!", message->peer->name, media_type_to_str(message->media_type));
            char * response = "Well nmea!!\x00";
            send_message_strings(message->peer, 0,0, (uint8_t *)response, 13, NULL);
        } else {
            ROB_LOGE(nmea_log_prefix, "A message, but not nmea: %s", message->strings[0]);
        }
    } else {
        ROB_LOGE(nmea_log_prefix, "Got a message that didn't have one string!");
    }
    
}

void shutdown_nmea_service(void) {

    ROB_LOGE(nmea_log_prefix, "nmea service shutdown.");
}


void shutdown_nmea_network_service(void) {

    ROB_LOGE(nmea_log_prefix, "nmea network service shutdown.");
}

void start_nmea_service(char * _log_prefix)
{
    if (robusto_register_network_service(&nmea_service) != ROB_OK) {
        ROB_LOGE(nmea_log_prefix, "Failed adding service");
    }
}

void init_nmea_service(char * _log_prefix)
{
    nmea_log_prefix = _log_prefix;

}

void register_nmea_service() {
    register_service(init_nmea_service, start_nmea_service, shutdown_nmea_service, 4,  "NMEA 2000 service");    
}
