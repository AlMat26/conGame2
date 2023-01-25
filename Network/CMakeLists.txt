cmake_minimum_required(VERSION 3.0)

project(networklib)

#############################
find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
    message(SEND_ERROR "Failed to find Boost")
    return()
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()
#############################

set(SOURCES_LIB network.cpp)
set(HEADERS_LIB network.h)

add_library(networklib STATIC ${SOURCES_LIB} ${HEADERS_LIB})

target_link_libraries(networklib ${Boost_LIBRARIES})
