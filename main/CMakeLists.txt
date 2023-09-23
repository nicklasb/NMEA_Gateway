FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS ${app_sources}
REQUIRES robusto-base robusto-network robusto-server robusto-ui nmea-service nmea-gateway

)
