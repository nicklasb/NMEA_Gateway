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



#define SPEED_THROUGH_WATER_PGN 128259UL
#define SET_EVO_PILOT_COURSE 126208UL
#define TARGET_HEADING_MAGNETIC 65360UL
#define HEADING_MAGNETIC 65359UL

void on_incoming(robusto_message_t *message);
void shutdown_nmea_network_service(void);

char *nmea_log_prefix;
char _service_name[26] = "NMEA 2000 network service";

uint16_t count_in = 0;
uint16_t count_out = 0;
uint16_t fail_in = 0;

#ifdef CONFIG_SIMULATE_AP
int32_t last_heading_magnetic = 0;
#endif

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
#ifdef CONFIG_ROBUSTO_UI_MINIMAL

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

    sprintf(&service_row, "S|I%-3dO%-3dF%-3d", count_in, count_out, fail_in);
    robusto_screen_minimal_write(service_row, 0, 3);
    char *nmea_string = get_nmea_state_string();
    robusto_screen_minimal_write(nmea_string, 0, 0);
    robusto_free(nmea_string);

#endif
}

void forward_to_NMEA_hdg(int32_t value, uint32_t pgn)
{
    pubsub_server_topic_t *topic = robusto_pubsub_server_find_or_create_topic("NMEA.hdg");
    if (topic)
    {
        ROB_LOGI(nmea_log_prefix, "Forwarding data to NMEA.hdg");

        uint8_t value_len = sizeof(pgn) + sizeof(value);
        uint8_t *value_data = robusto_malloc(value_len);
        memcpy(value_data, &pgn, sizeof(pgn));
        memcpy(value_data + sizeof(pgn), &value, sizeof(value));
        robusto_pubsub_server_publish(topic->hash, value_data, value_len);
        robusto_free(value_data);
    }
}

void nmea_monitor_cb()
{
    write_server_stats();

    // Target Heading magnetic
    // TODO: We are not rounding target up as we are gettings precision errors?
    forward_to_NMEA_hdg((int32_t)(get_target_heading_magnetic() +0.5), TARGET_HEADING_MAGNETIC);
    forward_to_NMEA_hdg((int32_t)(get_heading_magnetic() +0.5), HEADING_MAGNETIC);
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
        uint32_t pgn = *(uint32_t *)(data);
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, data, data_length);
        if (pgn == SET_EVO_PILOT_COURSE)
        {
            int32_t curr_heading =  *(int32_t *)(data + 4);
            int32_t change = *(int32_t *)(data + 8);
            // Filter  // TODO: This must be in relation to how long since last update.
            if (curr_heading  > get_target_heading_magnetic()+ 30 || curr_heading < get_target_heading_magnetic() - 30) {
                ROB_LOGE(nmea_log_prefix, "Target heading change larger than 30 degrees! Heading: %li, Change %li.", curr_heading, change);
                return;
            }
            if (change  > 20 || change < - 20) {
                ROB_LOGE(nmea_log_prefix, "Heading change larger than 20 degrees! Heading: %li, Change %li.", curr_heading, change);
                return;
            }


            NMEA2000_Controller_set_heading(curr_heading, change);
            ROB_LOGI(nmea_log_prefix, "Sent heading change from pubsub: Heading - %li, Change %li degrees!", curr_heading, change);
#ifdef CONFIG_SIMULATE_AP
            int32_t target_heading_magnetic;
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
            set_target_heading_magnetic(target_heading_magnetic);
            forward_to_NMEA_hdg((double)target_heading_magnetic, TARGET_HEADING_MAGNETIC);
            set_heading_magnetic((double)last_heading_magnetic);
            //forward_to_NMEA_hdg((double)last_heading_magnetic, HEADING_MAGNETIC);
            last_heading_magnetic = target_heading_magnetic;

#endif

            count_in++;
        }
        else
        {
            ROB_LOGE(nmea_log_prefix, "An unrecognized PGN: %lu", pgn);
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
    robusto_pubsub_server_find_or_create_topic("NMEA.hdg");
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
