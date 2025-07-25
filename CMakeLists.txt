cmake_minimum_required(VERSION 3.14)
project(Mist++ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(mist++ src/mist.cpp)
set_target_properties(PROPERTIES
  VERSION   0.1.0
  SOVERSION 0
)
target_include_directories(mist++ PUBLIC include)
target_link_libraries(mist++ PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

option(BUILD_TEST "Build the test project" ON)

if(BUILD_TEST)
  add_subdirectory(test)
endif()
