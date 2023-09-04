FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.* ${CMAKE_SOURCE_DIR}/include/*.*)

idf_component_register(SRCS ${app_sources}
INCLUDE_DIRS . ../include
REQUIRES nmea-gateway
)
