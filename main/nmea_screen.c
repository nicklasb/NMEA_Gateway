
#include "nmea_screen.h"
#include "screen.h"
#include "lvgl.h"
#include <robusto_logging.h>
static lv_obj_t *screen = NULL;

static lv_obj_t *nmea_stats;
static lv_obj_t *server_stats;

static char *nmea_screen_log_prefix;

void set_nmea_stats(const char *txt)
{
    label_set_text(nmea_stats, txt);
    //ROB_LOGE(nmea_screen_log_prefix, "NMEA set to %s", txt);
}

void set_server_stats(const char *txt)
{
    label_set_text(server_stats, txt);
    //ROB_LOGE(nmea_screen_log_prefix, "Server set to %s", txt);
}

void start_nmea_screen()
{
    // Initiate all the
    if (!screen)
    {
        ROB_LOGE(nmea_screen_log_prefix, "NMEA screen initializing");
        screen = get_current_screen();
        
        nmea_stats = lv_label_create(screen);
        label_set_text(nmea_stats, "N|");
        lv_obj_set_width(nmea_stats, 128);

        lv_obj_align(nmea_stats, LV_ALIGN_TOP_LEFT, 0, 0);

        server_stats = lv_label_create(screen);
        label_set_text(nmea_stats, "S|");
        lv_obj_set_width(server_stats, 128);
        lv_obj_align(server_stats, LV_ALIGN_TOP_LEFT, 0, 16);

    }
    ROB_LOGE(nmea_screen_log_prefix, "NMEA screen initialized");
}

void init_nmea_screen(char *_log_prefix)
{
    nmea_screen_log_prefix = _log_prefix;
    /*
        static lv_style_t large_font;
        lv_style_init(&large_font);
        lv_style_set_text_font(&large_font, LV_STATE_DEFAULT, &arial70px);
        */
}