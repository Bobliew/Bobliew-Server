cmake_minimum_required(VERSION 2.8)
project(bobliew)

include (cmake/utils.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O2 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(..)
include_directories(.)
include_directories(~/build)

link_directories(~/build/yaml-cpp)
#link_directories(/apps/bobliew/lib)
#link_directories(/apps/bobliew/lib64)


find_package(Protobuf)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_library(PTHREAD pthread)
find_package(Protobuf)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

set(LIB_SRC
    bobliew/log.cc
    bobliew/address.cc
    bobliew/bytearray.cc
    bobliew/config.cc
    bobliew/fd_manager.cc
    bobliew/util.cc
    bobliew/thread.cc
    bobliew/mutex.cc
    bobliew/fiber.cc
    bobliew/socket.cc
    bobliew/scheduler.cc
    bobliew/stream.cc
    bobliew/timer.cc
    bobliew/tcp_server.cc
    bobliew/iomanager.cc
    bobliew/hook.cc
    bobliew/streams/socket_stream.cc
    bobliew/streams/zlib_stream.cc
    bobliew/http/http.cc
    bobliew/http/http_server.cc
    #bobliew/http/httpclient_parser.cc
    #bobliew/http/http11_parser.cc
    bobliew/http/http_parser.cc
    bobliew/http/http_session.cc
    bobliew/http/http_connection.cc
    bobliew/http/servlet.cc
    bobliew/http/servlets/config_servlet.cc
    #bobliew/http/servlets/status_servlet.cc
    bobliew/util/json_util.cc
    )
ragelmaker(bobliew/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/bobliew/http)
ragelmaker(bobliew/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/bobliew/http)
ragelmaker(bobliew/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/bobliew)

add_library(bobliew SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(bobliew) #__FILE__
#add_library(bobliew_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (bobliew_static PROPERTIES OUTPUT_NAME "bobliew")

set(LIB_LIB
        dl
        bobliew
        pthread
        yaml-cpp
        jsoncpp
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
    )

add_executable(test tests/test.cc)
add_dependencies(test bobliew)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIB_LIB} dl)


add_executable(test_config tests/test_config.cc)
add_dependencies(test_config bobliew)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB} dl)

add_executable(test_configs tests/test_configs.cc)
add_dependencies(test_configs bobliew)
force_redefine_file_macro_for_sources(test_configs)
target_link_libraries(test_configs ${LIB_LIB} dl)

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util bobliew)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIB_LIB} dl)

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread bobliew)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_LIB} dl)

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler bobliew)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB} dl)



add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber bobliew)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB} dl)



add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager bobliew)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIB_LIB} dl)


add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook bobliew)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIB_LIB} dl)

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address bobliew)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIB_LIB} dl)

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray bobliew)
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIB_LIB} dl)


add_executable(test_http tests/test_http.cc)
add_dependencies(test_http bobliew)
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIB_LIB} dl)

add_executable(test_parser tests/test_parser.cc)
add_dependencies(test_parser bobliew)
force_redefine_file_macro_for_sources(test_parser)
target_link_libraries(test_parser ${LIB_LIB} dl)

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server bobliew)
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIB_LIB} dl)

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server bobliew)
force_redefine_file_macro_for_sources(test_http_server)
target_link_libraries(test_http_server ${LIB_LIB} dl)

add_executable(test_http_server2 tests/test_http_server2.cc)
add_dependencies(test_http_server2 bobliew)
force_redefine_file_macro_for_sources(test_http_server2)
target_link_libraries(test_http_server2 ${LIB_LIB} dl)

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection bobliew)
force_redefine_file_macro_for_sources(test_http_connection)
target_link_libraries(test_http_connection ${LIB_LIB} dl)

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri bobliew)
force_redefine_file_macro_for_sources(test_uri)
target_link_libraries(test_uri ${LIB_LIB} dl)

add_executable(my_http_server samples/my_http_server.cc)
add_dependencies(my_http_server bobliew)
force_redefine_file_macro_for_sources(my_http_server)
target_link_libraries(my_http_server ${LIB_LIB} dl)



add_executable(echo_server example/echo_server.cc)
add_dependencies(echo_server bobliew)
force_redefine_file_macro_for_sources(echo_server)
target_link_libraries(echo_server ${LIB_LIB} dl)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
