FILE(GLOB_RECURSE nmea_sources 
"include/*.h"
"src/*.cpp" 
)

idf_component_register(
    SRCS ${nmea_sources} 
    INCLUDE_DIRS . include
    REQUIRES robusto NMEA2000_esp32xx NMEA2000 driver
)

