FILE(GLOB nmea_sources 
"include/*.h"
"src/*.cpp" 
)

idf_component_register(
    SRCS ${nmea_sources} 
    INCLUDE_DIRS . include
    REQUIRES robusto NMEA2000 NMEA2000_esp32
)
