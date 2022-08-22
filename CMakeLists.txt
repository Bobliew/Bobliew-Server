cmake_minimum_required(VERSION 2.8)
project(bobliew)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(~/build)

link_directories(~/build/yaml-cpp)

find_library(PTHREAD pthread)

set(LIB_SRC
    bobliew/log.cc
    bobliew/config.cc
    bobliew/util.cc
    bobliew/thread.cc
    )

add_library(bobliew SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(bobliew) #__FILE__
#add_library(bobliew_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (bobliew_static PROPERTIES OUTPUT_NAME "bobliew")

set(LIB_LIB
    bobliew
    pthread
    yaml-cpp
    )


add_executable(test_config tests/test_config.cc)
add_dependencies(test_config bobliew)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread bobliew)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_LIB})



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
