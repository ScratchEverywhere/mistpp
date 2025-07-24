cmake_minimum_required(VERSION 3.14)
project(Mist++ VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG curl-8_10_1
)
FetchContent_MakeAvailable(curl)

add_library(mist++ STATIC src/mist.cpp)
target_include_directories(mist++ PUBLIC include)
target_link_libraries(mist++ PUBLIC curl_s)
