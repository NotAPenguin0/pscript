# Additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)

FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.0.0-preview3
)
FetchContent_MakeAvailable(catch2)

add_executable(plib-test main.cpp)
target_link_libraries(plib-test PRIVATE plib Catch2::Catch2WithMain)
target_compile_options(plib-test PRIVATE -Wno-macro-redefined -Wno-format)

if (${PLIB_BUILD_PSCRIPT_DEMO_APP})
    add_subdirectory("perceptron_demo")
endif()

file(GLOB_RECURSE MODULE_FILES "../modules/*")
add_custom_command(
        TARGET plib-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../modules/" ${CMAKE_CURRENT_BINARY_DIR}/pscript-modules
        DEPENDS ${MODULE_FILES}
        COMMENT "Copying plib modules - tests"
        VERBATIM
)

file(GLOB_RECURSE SAMPLES "../samples/*")
add_custom_command(
        TARGET plib-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../samples/" ${CMAKE_CURRENT_BINARY_DIR}/samples
        DEPENDS ${SAMPLES}
        COMMENT "Copying plib samples - tests"
        VERBATIM
)
