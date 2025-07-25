cmake_minimum_required(VERSION 3.10)

add_executable(mistpp_example main.cpp)

target_link_libraries(mistpp_example PRIVATE mist++)

target_include_directories(mistpp_example PRIVATE ${CMAKE_SOURCE_DIR}/include)
