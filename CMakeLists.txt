cmake_minimum_required(VERSION 4.2.0)

project(csc VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

if(UNIX)
    set(PLATFORM_NAME unix)
elseif(WIN32)
    set(PLATFORM_NAME windows)
else()
    message("not support system")
endif()

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/${PLATFORM_NAME})

file(
    GLOB_RECURSE SOURCE_FILE
    ${PROJECT_SOURCE_DIR}/src/common/*.cpp
    ${SOURCE_DIR}/*.cpp
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/log)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/cmd)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/argp)

add_executable(csc src/main.cpp)
target_link_libraries(csc PRIVATE log argp cmd)
