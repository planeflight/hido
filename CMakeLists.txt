cmake_minimum_required(VERSION 3.5)

project(karting)

# autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET (CMAKE_CXX_FLAGS "-Wall -Wextra -Wcast-qual -Wconversion-null -Wformat-security \
-Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wundef \
-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla \
-Wwrite-strings -DNOMINMAX -fno-omit-frame-pointer -fPIC -g")

set(CMAKE_CXX_STANDARD 20)

set(SRC
    src/server/server.cpp
    src/server/main.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
add_executable(client 
    src/client/client.cpp
    src/client/main.cpp
    src/car.cpp
)

include_directories(src/)


include(FindPkgConfig)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)

add_subdirectory(lib/raylib)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE fmt::fmt
    PRIVATE spdlog::spdlog
)

target_link_libraries(client
    PRIVATE fmt::fmt
    PRIVATE spdlog::spdlog
    PRIVATE raylib
)
