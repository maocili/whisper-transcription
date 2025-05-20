#include_directories("/usr/local/include" )
#link_directories("/usr/local/lib")

add_subdirectory(common)

#set(CMAKE_SKIP_BUILD_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH "/usr/local/lib")
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(TARGET src)

set(NODE_ADDON_API_DIR ${PROJECT_SOURCE_DIR}/node_modules/node-addon-api)
set(NODE_API_HEADERS_DIR ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/include)

include_directories(
    ${CMAKE_JS_INC}
    ${NODE_ADDON_API_DIR}
    ${NODE_API_HEADERS_DIR}
    ${PROJECT_INCLUDE}
    ${LIBUV_INCLUDE_DIR}
    ${LIBWHISPER_INCLUDE_DIR}
    common
    common-sdl
)

add_library(${TARGET} SHARED
        mq.cc
        model.cpp
        operator.cpp
)

target_link_libraries(${TARGET} PRIVATE
        ${WHISPER_LIBRARY}
        ${LIBUV_LIBRARIES}
        common
        common-sdl
)

