#include "handlers.h"
#include <robusto_incoming.h>
#include <robusto_logging.h>

char * handler_log_prefix;

void incoming_handler(incoming_queue_item_t *incoming_item) {
    ROB_LOGI(handler_log_prefix, "Got an incoming item");
    rob_log_bit_mesh(ROB_LOG_INFO, handler_log_prefix, incoming_item->message->raw_data, incoming_item->message->raw_data_length);
}

void init_handlers(char * _log_prefix) {
    handler_log_prefix = _log_prefix;
    if (robusto_register_handler(incoming_handler) != ROB_OK) {
        ROB_LOGE(handler_log_prefix, "Failed register the incoming handler");
    }
    ROB_LOGI(handler_log_prefix, "Registered the incoming handlers");
}