#include "nmea_service.h"

#include <robusto_init.h>
#include <robusto_logging.h>
#include <robusto_incoming.h>
#include <robusto_network_service.h>
#include <robusto_message.h>
#include <robusto_pubsub_server.h>

#include <NMEA2000Controller.h>
#include <robusto_incoming.h>
#ifdef CONFIG_ROBUSTO_UI_MINIMAL
#include <robusto_screen_minimal.h>
#endif
#include <robusto_repeater.h>

#include <string.h>

const uint16_t serviceid = 1959;

#define SPEED_THROUGH_WATER_PGN 128259L
#define SET_EVO_PILOT_COURSE 126208L
#define TARGET_HEADING_MAGNETIC 65360L

void on_incoming(robusto_message_t *message);
void shutdown_nmea_network_service(void);

char *nmea_log_prefix;
char _service_name[26] = "NMEA 2000 network service";

uint16_t count_in = 0;
uint16_t count_out = 0;
uint16_t fail_in = 0;

void nmea_monitor_cb();
void nmea_monitor_shutdown_cb();

char nmea_monitor_name[15] = "NMEA monitor";
recurrence_t nmea_monitor = {
    recurrence_name : &nmea_monitor_name,
    skip_count : 10,
    skips_left : 0,
    recurrence_callback : &nmea_monitor_cb,
    shutdown_callback : &nmea_monitor_shutdown_cb
};

void write_server_stats()
{
    // This is bigger than the available area on screen, we manually maximize output
    char service_row[23] = {0};
    if (count_in > 999)
    {
        count_in = 999;
    }
    if (count_out > 999)
    {
        count_out = 999;
    }
    if (fail_in > 999)
    {
        fail_in = 999;
    }
#ifdef CONFIG_ROBUSTO_UI_MINIMAL

    sprintf(&service_row, "S|I%-3dO%-3dF%-3d", count_in, count_out, fail_in);
    robusto_screen_minimal_write(service_row, 0, 3);
    char *nmea_string = get_nmea_state_string();
    robusto_screen_minimal_write(nmea_string, 0, 0);
    robusto_free(nmea_string);

#endif
}

void forward_to_NMEA_hdg(float target_heading_magnetic)
{
    pubsub_server_topic_t *topic = robusto_pubsub_server_find_or_create_topic("NMEA.hdg");
    if (topic)
    {
        ROB_LOGI(nmea_log_prefix, "Forwarding AP to NMEA.hdg");
        uint32_t thm_pgn = TARGET_HEADING_MAGNETIC;

        uint8_t target_heading_magnetic_len = sizeof(thm_pgn) + sizeof(target_heading_magnetic);
        uint8_t *target_heading_magnetic_data = robusto_malloc(target_heading_magnetic_len);
        memcpy(target_heading_magnetic_data, &thm_pgn, sizeof(thm_pgn));
        memcpy(target_heading_magnetic_data + sizeof(thm_pgn), &target_heading_magnetic, sizeof(target_heading_magnetic));
        robusto_pubsub_server_publish(topic->hash, target_heading_magnetic_data, target_heading_magnetic_len);
        robusto_free(target_heading_magnetic_data);
    }
}

void nmea_monitor_cb()
{
    write_server_stats();
#ifndef CONFIG_FORWARD_AP_TO_NMEA_HDG
    // Target Heading magnetic
    forward_to_NMEA_hdg((float)(get_target_heading_magnetic()));
#endif
}

void nmea_monitor_shutdown_cb()
{
}

void on_speed_publication(uint8_t *data, uint16_t data_length)
{
    ROB_LOGI(nmea_log_prefix, "In on_speed_publication");
    if (data_length > 0)
    {
        int32_t pgn = *(uint32_t *)(data);
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, data, data_length);
        if (pgn == SPEED_THROUGH_WATER_PGN)
        {
            double speed_through_water = (double)(*(uint16_t *)(data + sizeof(uint32_t))) / 1000;
            NMEA2000_Controller_send_speedThroughWater(speed_through_water);
            ROB_LOGI(nmea_log_prefix, "Sent speed from pubsub: %f knots!", speed_through_water);
            count_in++;
        }
        else
        {
            ROB_LOGE(nmea_log_prefix, "An unrecognized PGN: %lu", *(uint32_t *)(data));
            fail_in++;
        }
    }
    else
    {
        ROB_LOGE(nmea_log_prefix, "Got a pubsub message that didn't have any binary data!");
        fail_in++;
    }
}

void on_ap_publication(uint8_t *data, uint16_t data_length)
{
    ROB_LOGI(nmea_log_prefix, "In on_ap_publication");
    if (data_length > 0)
    {
        int32_t pgn = *(uint32_t *)(data);
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, data, data_length);
        if (pgn == SET_EVO_PILOT_COURSE)
        {
            float curr_heading = (float)(*(float *)(data + 4));
            int change = (int)(*(int *)(data + 8));
            NMEA2000_Controller_set_heading(curr_heading, change);
            ROB_LOGI(nmea_log_prefix, "Sent heading change from pubsub: Heading - %f, Change %i degrees!", curr_heading, change);
#ifdef CONFIG_FORWARD_AP_TO_NMEA_HDG
            float target_heading_magnetic;
            if ((curr_heading + change) < 0)
            {
                target_heading_magnetic = curr_heading + change + 360;
            }
            else if ((curr_heading + change) > 359)
            {
                target_heading_magnetic = curr_heading + change - 360;
            }
            else
            {
                target_heading_magnetic = curr_heading + change;
            }

            forward_to_NMEA_hdg(target_heading_magnetic);

#endif

            count_in++;
        }
        else
        {
            ROB_LOGE(nmea_log_prefix, "An unrecognized PGN: %lu", *(uint32_t *)(data));
            fail_in++;
        }
    }
    else
    {
        ROB_LOGE(nmea_log_prefix, "Got a pubsub message that didn't have any binary data!");
        fail_in++;
    }
}

void shutdown_nmea_service(void)
{

    ROB_LOGE(nmea_log_prefix, "nmea service shutdown.");
}

void shutdown_nmea_network_service(void)
{

    ROB_LOGE(nmea_log_prefix, "nmea network service shutdown.");
}

void start_nmea_service(void)
{

    robusto_pubsub_server_subscribe(NULL, &on_speed_publication, "NMEA.speed");
    robusto_pubsub_server_subscribe(NULL, &on_ap_publication, "NMEA.ap");
    robusto_register_recurrence(&nmea_monitor);
}

void init_nmea_service(char *_log_prefix)
{
    nmea_log_prefix = _log_prefix;
}

void register_nmea_service(void)
{
    char *tst = malloc(18);
    strcpy(tst, "NMEA 2000 service");
    register_service(init_nmea_service, start_nmea_service, shutdown_nmea_service, 4, tst);
}
