#include "screen.h"
#include <robusto_screen.h>
static char * ui_log_prefix;



void init_screen(char * _log_prefix) {
    ui_log_prefix = _log_prefix;
    robusto_screen_init(ui_log_prefix);
}