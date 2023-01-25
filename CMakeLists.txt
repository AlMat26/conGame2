cmake_minimum_required(VERSION 3.0)

project(congame)

##############################
find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
    message(SEND_ERROR "Failed to find Boost")
    return()
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()
##############################

add_executable(congame main.cpp)

add_subdirectory(ConRender)

target_link_libraries(congame conrenderlib ${Boost_LIBRARIES})

install(TARGETS congame RUNTIME DESTINATION bin)
