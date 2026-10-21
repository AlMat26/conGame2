cmake_minimum_required(VERSION 3.10)

project(conrenderlib)

##########################
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
if(NOT CURSES_FOUND)
    message(SEND_ERROR "Failed to find Curses")
    return()
else()
    include_directories(${CURSES_INCLUDE_DIRS})
endif()
##########################

set(SOURCE_LIB conrender.cpp)
set(HEADER_LIB conrender.h)

add_library(conrenderlib STATIC ${SOURCE_LIB} ${HEADER_LIB})

target_link_libraries(conrenderlib ${CURSES_LIBRARIES})
