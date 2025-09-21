cmake_minimum_required(VERSION 3.5)

project(hido)

# autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET (CMAKE_CXX_FLAGS "-Wall -Wextra -Wcast-qual -Wconversion-null -Wformat-security \
-Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wundef \
-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla \
-Wwrite-strings -DNOMINMAX -fno-omit-frame-pointer -fPIC -g")

set(CMAKE_CXX_STANDARD 20)

# enforce static linking
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

set(SRC
    src/server/server.cpp
    src/server/client_manager.cpp
    src/server/main.cpp
    src/map/map.cpp
    src/state/player.cpp
    src/state/bullet.cpp
    src/network.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
add_executable(client 
    src/client/client.cpp
    src/client/main.cpp
    src/map/map.cpp
    src/map/map_renderer.cpp
    src/state/player.cpp
    src/state/bullet.cpp
    src/network.cpp
)

include_directories(src/)
include_directories(lib/)


include(FindPkgConfig)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)
set(FMT_INSTALL OFF)
set(FMT_TEST OFF)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
set(SPDLOG_INSTALL OFF)
set(SPDLOG_BUILD_SHARED OFF)
FetchContent_MakeAvailable(spdlog)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(lib/raylib)
add_subdirectory(lib/libtmx-parser)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE fmt::fmt
    PRIVATE spdlog::spdlog
    PRIVATE raylib
    PRIVATE libtmx-parser
)

target_link_libraries(client
    PRIVATE fmt::fmt
    PRIVATE spdlog::spdlog
    PRIVATE raylib
    PRIVATE libtmx-parser
)
