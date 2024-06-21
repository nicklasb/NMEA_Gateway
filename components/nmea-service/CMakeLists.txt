FILE(GLOB_RECURSE nmea_service 
"include/*.h"
"src/*.c" 
)

idf_component_register(
    SRCS ${nmea_service} 
    INCLUDE_DIRS . include
    REQUIRES robusto nmea-gateway
)

