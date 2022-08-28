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
    bobliew/fiber.cc
    bobliew/scheduler.cc
    bobliew/iomanager.cc
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

add_executable(test tests/test.cc)
add_dependencies(test bobliew)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIB_LIB})


add_executable(test_config tests/test_config.cc)
add_dependencies(test_config bobliew)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_configs tests/test_configs.cc)
add_dependencies(test_configs bobliew)
force_redefine_file_macro_for_sources(test_configs)
target_link_libraries(test_configs ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util bobliew)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread bobliew)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler bobliew)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB})



add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber bobliew)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB})



add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager bobliew)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIB_LIB})



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
