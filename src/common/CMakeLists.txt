set(TARGET common)

include_directories("/usr/local/include")
link_directories("/usr/local/lib")

add_library(${TARGET} STATIC
        common.h
        common.cpp
        common-ggml.h
        common-ggml.cpp
        common-whisper.h
        common-whisper.cpp
        grammar-parser.h
        grammar-parser.cpp
)

# SDL2
#if (WHISPER_SDL2)
set(TARGET common-sdl)

# SDL2
find_package(SDL2 REQUIRED)

string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_LIBRARIES    = ${SDL2_LIBRARIES}")


add_library(${TARGET} STATIC
        common-sdl.h
        common-sdl.cpp
)

target_include_directories(${TARGET} PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(${TARGET} PRIVATE ${SDL2_LIBRARIES})
#endif ()

## whisper
#set(TARGET whisper)
#target_include_directories(${TARGET} PUBLIC "/usr/local/include")
#target_link_libraries(${TARGET} PRIVATE "/usr/local/lib")