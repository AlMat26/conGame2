cmake_minimum_required(VERSION 3.0)

project(conrenderlib)

##########################
find_package(Curses REQUIRED)
if(NOT Curses_FOUND)
    message(SEND_ERROR "Failed to find Curses")
    return()
else()
    include_directories(${Curses_INCLUDE_DIRS})
endif()
##########################

set(SOURCE_LIB conrender.cpp)
set(HEADER_LIB conrender.h)

add_library(conrenderlib STATIC ${SOURCE_LIB} ${HEADER_LIB} ${Curses_LIBRARIES})

target_link_libraries(conrenderlib ncurses)
