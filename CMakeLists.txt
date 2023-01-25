cmake_minimum_required(VERSION 3.0)

project(congame)

add_executable(congame main.cpp)

add_subdirectory(ConRender)
add_subdirectory(Network)

target_link_libraries(congame conrenderlib networklib)

install(TARGETS congame RUNTIME DESTINATION bin)
