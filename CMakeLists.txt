cmake_minimum_required(VERSION 3.0)

project(congame)

add_executable(congame main.cpp)

install(TARGETS congame RUNTIME DESTINATION bin)
