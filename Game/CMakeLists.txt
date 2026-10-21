cmake_minimum_required(VERSION 3.10)

project(gamelib)

set(SOURCES_LIB game.cpp)
set(HEADERS_LIB game.h)

add_library(gamelib STATIC ${SOURCES_LIB} ${HEADERS_LIB})

target_link_libraries(gamelib conrenderlib worldlib networklib)
