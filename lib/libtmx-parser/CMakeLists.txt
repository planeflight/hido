cmake_minimum_required(VERSION 3.5)

project(libtmx-parser)

set (CMAKE_CXX_FLAGS "-std=c++20")

add_library(${PROJECT_NAME} SHARED 
    tmxparser.cpp
    base64.cpp
    tinyxml2/tinyxml2.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2
)
