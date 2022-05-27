cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(plib)

set(CMAKE_CXX_STANDARD 20)

set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

option(PLIB_ENABLE_TESTS "Enable building tests" ${is_root_project})

if(PLIB_ENABLE_TESTS)
  add_subdirectory(tests)
endif()

# TODO: Add option to exclude pscript

add_library(plib STATIC)
target_sources(plib PRIVATE src/pscript/context.cpp src/pscript/memory.cpp src/pscript/value.cpp src/pscript/variable.cpp)
target_include_directories(plib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# TODO: clang
target_compile_options(plib PRIVATE /EHsc)