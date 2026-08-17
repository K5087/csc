set(CMD_DIR ${SOURCE_DIR}/cmd)

set(CMD_SOURCES cmd.cpp ${CMD_DIR}/cmd.cpp)

add_library(cmd STATIC ${CMD_SOURCES})
target_include_directories(cmd PUBLIC ${INCLUDE_DIR})
target_link_libraries(cmd PRIVATE log)

# add_library(cmd_shared SHARED ${CMD_SOURCES})
# target_include_directories(cmd_shared PUBLIC ${INCLUDE_DIR})
# target_link_libraries(cmd_shared PRIVATE log_static)
