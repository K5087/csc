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

set(RSC_DIR ${SOURCE_DIR}/rsc)
set(RSC_SOURCES src/main.cpp src/common/rsc/rsc.cpp src/windows/rsc/os.cpp)
set(RSC_TEST_SOURCES compile.cpp src/common/rsc/rsc.cpp src/windows/rsc/os.cpp)


add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/log)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/cmd)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/argp)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common/csc)

add_executable(rsc ${RSC_SOURCES})
add_executable(rsc_test ${RSC_TEST_SOURCES})
target_link_libraries(rsc PRIVATE log argp cmd csc)
target_link_libraries(rsc_test PRIVATE log argp cmd csc)
