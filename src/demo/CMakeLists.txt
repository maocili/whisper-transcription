add_executable(demo1
        demo1.cpp
)

target_include_directories(demo1 PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${LIBUV_INCLUDE_DIR}
        ${LIBWHISPER_INCLUDE_DIR}
)

target_link_libraries(demo1 PRIVATE
        ${WHISPER_LIBRARY}
        ${LIBUV_LIBRARIES}
        src
        common
        common-sdl
)