set(CSC_DIR ${SOURCE_DIR}/csc)

set(CSC_SOURCES csc.cpp target.cpp tool_chain.cpp ${CSC_DIR}/csc.cpp ${CSC_DIR}/tool_chain.cpp)

add_library(csc STATIC ${CSC_SOURCES})
target_include_directories(csc PUBLIC ${INCLUDE_DIR})
target_link_libraries(csc PRIVATE log cmd)
