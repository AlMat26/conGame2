cmake_minimum_required(VERSION 3.10)

project(congame)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(congame main.cpp)

add_subdirectory(ConRender)
add_subdirectory(Network)
add_subdirectory(World)
add_subdirectory(Game)

target_link_libraries(congame gamelib conrenderlib networklib worldlib)

install(TARGETS congame RUNTIME DESTINATION bin)
