cmake_minimum_required(VERSION 3.10)

project(worldlib)

set(SOURCES_LIB world.cpp)
set(HEADERS_LIB world.h)

add_library(worldlib STATIC ${SOURCES_LIB} ${HEADERS_LIB})
