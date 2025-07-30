cmake_minimum_required(VERSION 3.14)
project(Mist++ VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries (OFF for static)")
if(BUILD_SHARED_LIBS)
  add_library(mist++ SHARED src/mist.cpp)
else()
  add_library(mist++ STATIC src/mist.cpp)
endif()

set_target_properties(PROPERTIES
  VERSION   0.3.0
  SOVERSION 0
)

target_include_directories(mist++ PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(mist++ PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

install(TARGETS mist++
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/mist++.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/mist++.pc"
  @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mist++.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

option(BUILD_TEST "Build the test project" ON)

if(BUILD_TEST)
  add_subdirectory(test)
endif()
