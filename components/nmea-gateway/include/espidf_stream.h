#include <N2kStream.h>

class EspIDFStream: public N2kStream {
    public:    
        int read();
        int peek(); 

        // Write data to stream.
        size_t write(const uint8_t* data, size_t size);
};
