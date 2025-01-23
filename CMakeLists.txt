cmake_minimum_required(VERSION 2.8)
project(sylar)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/sylar
)

set(LIB_SRC
    sylar/address.cc
    sylar/bytearray.cc
    sylar/config.cc
    sylar/fiber.cc
    sylar/fd_manager.cc
    sylar/hook.cc
    sylar/iomanager.cc
    sylar/log.cc
    sylar/scheduler.cc
    sylar/socket.cc
    sylar/thread.cc
    sylar/timer.cc
    sylar/util.cc
)

add_library(sylar SHARED ${LIB_SRC})

find_package(yaml-cpp REQUIRED)

set(LIBS
    sylar
    yaml-cpp
    pthread
)

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log ${LIBS})
force_redefine_file_macro_for_sources(test_log)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIBS})
force_redefine_file_macro_for_sources(test_config)

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})
force_redefine_file_macro_for_sources(test_thread)

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIBS})
force_redefine_file_macro_for_sources(test_util)

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIBS})
force_redefine_file_macro_for_sources(test_fiber)

add_executable(test_scheduler tests/test_scheduler.cc ${LIB_SRC})
# add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIBS})
force_redefine_file_macro_for_sources(test_scheduler)

add_executable(test_iomanager tests/test_iomanager.cc ${LIB_SRC})
target_link_libraries(test_iomanager ${LIBS})
force_redefine_file_macro_for_sources(test_iomanager)

add_executable(test_hook tests/test_hook.cc ${LIB_SRC})
target_link_libraries(test_hook ${LIBS})
force_redefine_file_macro_for_sources(test_hook)

add_executable(test_address tests/test_address.cc ${LIB_SRC})
target_link_libraries(test_address ${LIBS})
force_redefine_file_macro_for_sources(test_address)

add_executable(test_socket tests/test_socket.cc ${LIB_SRC})
target_link_libraries(test_socket ${LIBS})
force_redefine_file_macro_for_sources(test_socket)

add_executable(test_bytearray tests/test_bytearray.cc ${LIB_SRC})
target_link_libraries(test_bytearray ${LIBS})
force_redefine_file_macro_for_sources(test_bytearray)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
