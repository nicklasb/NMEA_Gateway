#include "../include/espidf_stream.h"
#include <robusto_logging.h>
#include <string.h>

int currPos = 0;
#define STREAM_BUF_SIZE 1000

char buffer[STREAM_BUF_SIZE];

int EspIDFStream::read() {
    return 0;
}
int EspIDFStream::peek() {
    return 0;
}

// Write data to stream.
size_t EspIDFStream::write(const uint8_t *data, size_t size){
    if (size > STREAM_BUF_SIZE - currPos) {
        ROB_LOGI("DEBUG", "%*s", currPos, buffer);
        memcpy(buffer, data, size);
        currPos = size;

    } else {
        memcpy(buffer+currPos, data, size);
        currPos+= size;
    }

    return size;
}
