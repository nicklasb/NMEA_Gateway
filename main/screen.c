#include "screen.h"
#include <robusto_screen.h>
#include <lvgl.h>
#include <robusto_logging.h>

static char *screen_log_prefix;
static lv_disp_t *display;
static lv_obj_t *screen;

lv_obj_t *get_current_screen()
{
    return screen;
}

void robusto_screen_minimal_write_xy(char *txt, uint16_t x, uint16_t y, uint8_t height)
{
}
void robusto_screen_minimal_write(char *txt, uint8_t col, uint8_t row)
{
    robusto_screen_minimal_write_xy(txt, col * 8, row * 16, 16);
}

void robusto_screen_minimal_write_small(char *txt, uint8_t col, uint8_t row)
{
    robusto_screen_minimal_write_xy(txt, col * 8, row * 12, 12);
}

void label_set_text(lv_obj_t * label, const char * txt) {
    if (label) {
        if (robusto_screen_lvgl_port_lock(2000)) {
        lv_label_set_text(label, txt);
        robusto_screen_lvgl_port_unlock();
        } else {
            ROB_LOGE(screen_log_prefix, "Failed to get a lock on the LVGL port in 2000 ms");
        }
    }
    
}


void init_screen(char *_log_prefix)
{
    screen_log_prefix = _log_prefix;
    robusto_screen_init(screen_log_prefix);
    
    display = robusto_screen_lvgl_get_active_display();
    screen = lv_disp_get_scr_act(display);
}