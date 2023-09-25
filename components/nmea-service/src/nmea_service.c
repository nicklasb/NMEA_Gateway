#include "nmea_service.h"

#include <robusto_init.h>
#include <robusto_logging.h>
#include <robusto_incoming.h> 
#include <robusto_network_service.h>
#include <robusto_message.h>

#include <NMEA2000Controller.h>
#include <robusto_incoming.h> 
#ifdef CONFIG_ROBUSTO_UI_MINIMAL
#include <robusto_ui_minimal.h>
#endif
#include <robusto_repeater.h>

#include <string.h>

const uint16_t serviceid = 1959;

#define SPEED_THROUGH_WATER_PGN 128259L
#define SET_EVO_PILOT_COURSE 126208L

void on_incoming(robusto_message_t *message);
void shutdown_nmea_network_service(void) ;


char * nmea_log_prefix;
char _service_name[26] = "NMEA 2000 network service";

uint16_t count_in = 0;
uint16_t count_out = 0;
uint16_t fail_in = 0;
void nmea_monitor_cb();
void nmea_monitor_shutdown_cb();
network_service_t nmea_service = {
    service_id : serviceid,
    service_name : &_service_name,
    incoming_callback : &on_incoming,
    shutdown_callback: &shutdown_nmea_network_service
};

char nmea_monitor_name[15] = "NMEA monitor";
recurrence_t nmea_monitor = {
    recurrence_name : &nmea_monitor_name,
    skip_count : 10,
    skips_left : 0,
    recurrence_callback : &nmea_monitor_cb,
    shutdown_callback : &nmea_monitor_shutdown_cb
};

void write_server_stats() {
    // This is bigger than the available area on screen, we manually maximize output
    char service_row[23] =  {0};
    if (count_in > 999) {
        count_in = 999;
    }
    if (count_out > 999) {
        count_out = 999;
    }
    if (fail_in > 999) {
        fail_in = 999;
    }
#ifdef CONFIG_ROBUSTO_UI_MINIMAL


    sprintf(&service_row, "S|I%-3dO%-3dF%-3d", count_in, count_out, fail_in);
    robusto_ui_minimal_write(service_row, 0, 3);  
    char * nmea_string = get_nmea_state_string();
    robusto_ui_minimal_write(nmea_string, 0, 0);  
    robusto_free(nmea_string);

#endif
}


void nmea_monitor_cb()
{
    write_server_stats();
}

void nmea_monitor_shutdown_cb()
{
}

void on_incoming(robusto_message_t *message) {
    
    if (message->binary_data_length > 0) {
        int32_t curr_value = *(uint32_t *)(message->binary_data);
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, message->binary_data, message->binary_data_length);
        if (curr_value == SPEED_THROUGH_WATER_PGN) {
            double speed_through_water =  (double)(*(uint16_t *)(message->binary_data + sizeof(uint32_t)))/100;
            NMEA2000_Controller_send_speedThroughWater(speed_through_water);
            ROB_LOGI(nmea_log_prefix, "Sent speed from %s: %f knots!", message->peer->name, speed_through_water);
            count_in++;
        }
        else if (curr_value == SET_EVO_PILOT_COURSE) {
            
            float curr_heading = (float)(*(float *)(message->binary_data + 4));
            int change =  (int)(*(int *)(message->binary_data + 8));
            NMEA2000_Controller_set_heading(curr_heading, change);
            ROB_LOGI(nmea_log_prefix, "Sent heading change from %s: Heading - %f, Change %i degrees!", message->peer->name, curr_heading,change);
            count_in++;
        } else {
            ROB_LOGE(nmea_log_prefix, "An unrecognized PGN: %lu", *(uint32_t *)(message->binary_data));
            fail_in++;
        }
    } else {
        ROB_LOGE(nmea_log_prefix, "Got a message that didn't have any binary data!");
        fail_in++;
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
    robusto_register_recurrence(&nmea_monitor);

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
