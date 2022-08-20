cmake_minimum_required(VERSION 2.8)
project(bobliew)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)
include_directories(~/build)

link_directories(~/build/yaml-cpp)

set(LIB_SRC
    bobliew/log.cc
    bobliew/config.cc
    )

add_library(bobliew SHARED ${LIB_SRC})
#add_library(bobliew_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (bobliew_static PROPERTIES OUTPUT_NAME "bobliew")

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config bobliew)
target_link_libraries(test_config bobliew yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
