cmake_minimum_required(VERSION 3.10)
project(my-electron-app VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_INCLUDE ${PROJECT_SOURCE_DIR}/include)

# Base settings
#==================================================================
# env var supported by cmake-js
add_definitions(-DNAPI_VERSION=8)
add_definitions(-DNAPI_CPP_EXCEPTIONS=on)

# find libwhisper
find_path(LIBWHISPER_INCLUDE_DIR
        NAMES whisper.h
        PATHS /usr/local/include
        REQUIRED
)
find_library(WHISPER_LIBRARY
        NAMES whisper libwhisper
        PATHS /usr/local/lib
        REQUIRED
)

# find libuv
find_path(LIBUV_INCLUDE_DIR
        NAMES uv.h
        REQUIRED
)
find_library(LIBUV_LIBRARIES
        NAMES uv libuv
        REQUIRED
)

message(STATUS "LIBWHISPER_DIR: ${LIBWHISPER_INCLUDE_DIR}")
message(STATUS "LIBUV_LIBRARIES: ${LIBUV_LIBRARIES}")
message(STATUS "WHISPER_LIBRARY: ${WHISPER_LIBRARY}")

message(STATUS "LIBUV_DIR: ${LIBUV_INCLUDE_DIR}")
message(STATUS "LIBUV_LIBRARIES: ${LIBUV_LIBRARIES}")

add_subdirectory(src)

set(TARGET addon.node)

set(NODE_ADDON_API_DIR ${PROJECT_SOURCE_DIR}/node_modules/node-addon-api)
set(NODE_API_HEADERS_DIR ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/include)
message(STATUS "CMAKE_JS_INC: ${CMAKE_JS_INC}")
message(STATUS "CMAKE_JS_LIB: ${CMAKE_JS_LIB}")
message(STATUS "NODE_ADDON_API_DIR: ${NODE_ADDON_API_DIR}")
message(STATUS "NODE_API_HEADERS_DIR: ${NODE_API_HEADERS_DIR}")

include_directories(
        ${CMAKE_JS_INC}
        ${NODE_ADDON_API_DIR}
        ${PROJECT_INCLUDE}
        ${NODE_API_HEADERS_DIR}
        ${LIBWHISPER_INCLUDE_DIR}
)
#==================================================================
# Create the library target first
#==================================================================
set(ADDON_SOURCES
        ${PROJECT_SOURCE_DIR}/src/operator_wrapper.cpp
)

add_library(${TARGET} SHARED ${ADDON_SOURCES})

# Include N-API wrappers after target is created
#==================================================================
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

#==================================================================

set_target_properties(${TARGET} PROPERTIES PREFIX "" SUFFIX ".node")

target_link_libraries(${TARGET} ${CMAKE_JS_LIB} ${CMAKE_THREAD_LIBS_INIT} src)

if (MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    # Generate node.lib
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif ()


add_subdirectory(src/demo)
