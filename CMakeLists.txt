cmake_minimum_required(VERSION 2.8)
project(sylar)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
)

add_library(sylar SHARED ${LIB_SRC})

find_package(yaml-cpp REQUIRED)

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar yaml-cpp)
force_redefine_file_macro_for_sources(test_log)


add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar yaml-cpp)
force_redefine_file_macro_for_sources(test_config)



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)



# cmake_minimum_required(VERSION 2.8)
# project(sylar)
# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
# file(GLOB_RECURSE SRC sylar/*.h sylar/*.cc)
# add_executable(sylar ${SRC})
