set(ARGP_DIR ${SOURCE_DIR}/argp)

set(ARGP_SOURCES argp.cpp)

add_library(argp STATIC ${ARGP_SOURCES})
target_include_directories(argp PUBLIC ${INCLUDE_DIR})
#
# add_library(argp_shared SHARED ${ARGP_SOURCES})
# target_include_directories(argp_shared PUBLIC ${INCLUDE_DIR})
