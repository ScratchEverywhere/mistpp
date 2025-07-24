cmake_minimum_required(VERSION 3.14)
project(Mist++ VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

find_package(CURL REQUIRED)

add_library(mist++ src/mist.cpp)
target_include_directories(mist++ PUBLIC include)
target_link_libraries(mist++ PRIVATE CURL::libcurl)
