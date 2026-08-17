set(LOG_DIR ${SOURCE_DIR}/log)

set(LOG_SOURCES log.cpp)

add_library(log STATIC ${LOG_SOURCES})
target_include_directories(log PUBLIC ${INCLUDE_DIR})
#
# add_library(log_shared SHARED ${LOG_SOURCES})
# target_include_directories(log_shared PUBLIC ${INCLUDE_DIR})
