cmake_minimum_required(VERSION 2.8)
project(bobliew)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    bobliew/log.cc
    )

add_library(bobliew SHARED ${LIB_SRC})
#add_library(bobliew_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (bobliew_static PROPERTIES OUTPUT_NAME "bobliew")

add_executable(test tests/test.cc)
add_dependencies(test bobliew)
target_link_libraries(test bobliew)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
