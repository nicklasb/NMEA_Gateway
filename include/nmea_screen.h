#pragma once

#include <robconfig.h>
#ifdef CONFIG_ROBUSTO_UI
void set_nmea_stats(const char *txt);

void set_server_stats(const char *txt);

void start_nmea_screen();

void init_nmea_screen(char *_log_prefix);
#endif