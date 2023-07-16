FILE(GLOB rob_sources 
"include/*.h"
"src/*.c" 
)

idf_component_register(
    SRCS ${rob_sources} 
    INCLUDE_DIRS . include 
    REQUIRES robusto-base robusto-network robusto-concurrency NMEA2000_esp32 NMEA2000
)
