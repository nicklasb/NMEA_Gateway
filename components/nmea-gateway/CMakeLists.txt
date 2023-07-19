FILE(GLOB rob_sources 
"include/*.h"
"src/*.c" 
)

idf_component_register(
    SRCS ${rob_sources} 
    INCLUDE_DIRS . include
    REQUIRES robusto-base NMEA2000 NMEA2000_esp32
)
