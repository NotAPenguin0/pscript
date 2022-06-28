cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(FetchContent)

project(pscript)

set(CMAKE_CXX_STANDARD 20)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHa")
endif()

set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

option(PSCRIPT_ENABLE_TESTS "Enable building tests" ${is_root_project})
option(PSCRIPT_BUILD_DEMO_APP "Enable pscript demo app" ON) # TODO: set default to off

set(PEGLIB_BUILD_TESTS OFF)

if(${PSCRIPT_ENABLE_TESTS})
  enable_testing()
  add_subdirectory(tests)
endif()

FetchContent_Declare(
  peglib
  GIT_REPOSITORY https://github.com/NotAPenguin0/cpp-peglib
  GIT_TAG fed85fe14d0d2cf83fc649a72bd55a50f28439b2
)

FetchContent_MakeAvailable(peglib)

FetchContent_Declare(
        plib
        GIT_REPOSITORY https://github.com/NotAPenguin0/plib
)

FetchContent_MakeAvailable(plib)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
)
FetchContent_MakeAvailable(fmt)

add_library(pscript STATIC)
target_sources(pscript PRIVATE
        src/pscript/context.cpp
        src/pscript/memory.cpp
        src/pscript/value.cpp
        src/pscript/variable.cpp
        src/pscript/script.cpp
)
target_include_directories(pscript PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${peglib_SOURCE_DIR})


target_link_libraries(pscript PUBLIC fmt plib)
target_include_directories(pscript PUBLIC ${fmt_SOURCE_DIR}/include)
target_compile_options(pscript PRIVATE "-Wno-format")

# copy over modules to build directory
file(GLOB_RECURSE MODULE_FILES "modules/*")
add_custom_command(
        TARGET pscript POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/modules/" ${${CMAKE_PROJECT_NAME}_BINARY_DIR}/pscript-modules
        DEPENDS ${MODULE_FILES}
        COMMENT "Copying pscript modules"
        VERBATIM
)
