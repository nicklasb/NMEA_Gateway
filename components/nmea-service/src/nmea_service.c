#include "../include/nmea_service.h"

#include <robusto_init.h>
#include <robusto_logging.h>
#include <robusto_incoming.h>
#include <robusto_network_service.h>
#include <robusto_message.h>
#include <robusto_time.h>
#include <robusto_pubsub_server.h>

#include <NMEA2000Controller.h>
#include <robusto_incoming.h>
#ifdef CONFIG_ROBUSTO_UI_MINIMAL
#include <robusto_screen_minimal.h>
#endif
#include <robusto_repeater.h>

#include <string.h>
// Raymarine specific
// AP commands
#define SET_EVO_PILOT_COURSE 126208UL
// AP output
#define TARGET_HEADING_TRUE 65360UL
#define TARGET_HEADING_MAGNETIC 653601UL // Own differentiator
#define HEADING_MAGNETIC 65359UL
#define HEADING_TRUE 653591UL // Own differentiator
#define SPEED_THROUGH_WATER_PGN 128259UL
#define SPEED_COURSE_OVER_GROUND 129026UL
#define PILOT_STATE 65379UL
void shutdown_nmea_network_service(void);

static char *nmea_log_prefix;
static char _service_name[26] = "NMEA 2000 network service";

static ui_cb *cb_stats;
static ui_cb *cb_nmea;

static uint16_t count_in = 0;
static uint16_t count_out = 0;
static uint16_t fail_in = 0;

uint64_t last_heading_magnetic = 0;
uint64_t last_target_heading_magnetic = 0;


#ifdef CONFIG_SIMULATE_AP
int32_t last_heading_magnetic = 0;
#endif

void nmea_monitor_cb();
void nmea_monitor_shutdown_cb();

char nmea_monitor_name[15] = "NMEA monitor";
recurrence_t nmea_monitor = {
    recurrence_name : &nmea_monitor_name,
    skip_count : 20,
    skips_left : 0,
    recurrence_callback : &nmea_monitor_cb,
    shutdown_callback : &nmea_monitor_shutdown_cb
};

void write_server_stats()
{
#ifdef CONFIG_ROBUSTO_UI

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

    sprintf(&service_row, "S|I%-3d O%-3d F%-3d", count_in, count_out, fail_in);
    cb_nmea(&service_row);
    char *nmea_string = get_nmea_state_string();
    cb_stats(nmea_string);
    robusto_free(nmea_string);

#endif
}

void forward_to_topic(int32_t value, uint32_t pgn, char * topic_name)
{
    pubsub_server_topic_t *topic = robusto_pubsub_server_find_or_create_topic(topic_name);
    if (topic)
    {
        ROB_LOGI(nmea_log_prefix, "Forwarding data to %s. pgn = %lu", topic_name, pgn);

        uint8_t value_len = sizeof(pgn) + sizeof(value);
        uint8_t *value_data = robusto_malloc(value_len);
        memcpy(value_data, &pgn, sizeof(pgn));
        memcpy(value_data + sizeof(pgn), &value, sizeof(value));
        robusto_pubsub_server_publish(topic->hash, value_data, value_len);
        robusto_free(value_data);
    }
    else
    {
        ROB_LOGE(nmea_log_prefix, "Could not find or create topic NMEA.hdg");
    }
}

void nmea_message_callback(int32_t value, uint32_t pgn)
{
    if (pgn == TARGET_HEADING_MAGNETIC) {
        if (r_millis() > last_target_heading_magnetic + 1000) {
            last_target_heading_magnetic = r_millis();
            forward_to_topic(value, pgn, "NMEA.hdg");
        } else {
            return;
        }
    } else
    if (pgn == HEADING_MAGNETIC) {
        if (r_millis() > last_heading_magnetic + 1000) {
            last_heading_magnetic = r_millis();
            forward_to_topic(value, pgn, "NMEA.hdg");
        } else {
            return;
        }
    } else
    if (pgn == PILOT_STATE ) {
        forward_to_topic(value, pgn, "NMEA.ap_out");
    }
}

void nmea_monitor_cb()
{
    write_server_stats();

// Target Heading magnetic
// TODO: We are not rounding target up as we are gettings precision errors?
#if CONFIG_SIMULATE_AP
    // Initialize with some value
    forward_to_NMEA_hdg((int32_t)(get_target_heading_magnetic() + 0.5), TARGET_HEADING_MAGNETIC);
    forward_to_NMEA_hdg((int32_t)(get_heading_magnetic() + 0.5), HEADING_MAGNETIC);
#endif
}

void nmea_monitor_shutdown_cb()
{
}

void on_speed_publication(uint8_t *data, uint16_t data_length)
{
    ROB_LOGD(nmea_log_prefix, "In on_speed_publication");
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
    ROB_LOGD(nmea_log_prefix, "In on_ap_publication");
    if (data_length > 0)
    {
        uint32_t pgn = *(uint32_t *)(data);
        rob_log_bit_mesh(ROB_LOG_INFO, nmea_log_prefix, data, data_length);
        if (pgn == SET_EVO_PILOT_COURSE)
        {
            int32_t curr_heading = *(int32_t *)(data + 4);
            int32_t change = *(int32_t *)(data + 8);
            int32_t adj_heading = curr_heading;
            if (get_target_heading_magnetic() < 30)
            {
                if (curr_heading > 330)
                {
                    adj_heading = curr_heading - 360;
                }
            }

            // Filter  // TODO: This must be in relation to how long since last update.
            if (adj_heading > get_target_heading_magnetic() + 30 || adj_heading < get_target_heading_magnetic() - 30)
            {
                ROB_LOGE(nmea_log_prefix, "Target heading change larger than 30 degrees! Heading: %li, Change %f. Mag %li",
                         curr_heading, get_target_heading_magnetic(), change);
                return;
            }
            if (change > 20 || change < -20)
            {
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
            // forward_to_NMEA_hdg((double)last_heading_magnetic, HEADING_MAGNETIC);
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


void register_nmea_service_message_callback(void) {
    set_callback(nmea_message_callback);   
}

void start_nmea_service(void)
{

    robusto_pubsub_server_subscribe(NULL, &on_speed_publication, "NMEA.speed");
    robusto_pubsub_server_subscribe(NULL, &on_ap_publication, "NMEA.ap_in");
    robusto_pubsub_server_find_or_create_topic("NMEA.ap_out");
    robusto_pubsub_server_find_or_create_topic("NMEA.hdg");

    robusto_register_recurrence(&nmea_monitor);
}

void set_cb_nmea_service(ui_cb *_cb_stats, ui_cb *_cb_nmea)
{
    cb_stats = _cb_stats;
    cb_nmea = _cb_nmea;
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
