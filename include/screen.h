#pragma once

#include <robconfig.h>
#include <stdint.h>
#include <lvgl.h>

lv_obj_t * get_current_screen();

void label_set_text(lv_obj_t * label, const char * txt);


void robusto_screen_minimal_write_xy(char * txt, uint16_t x, uint16_t y, uint8_t height);
void robusto_screen_minimal_write(char * txt, uint8_t col, uint8_t row);
void robusto_screen_minimal_write_small(char * txt, uint8_t col, uint8_t row);
void init_screen(char * _log_prefix);