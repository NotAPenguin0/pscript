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
target_compile_options(plib-test PRIVATE -Wno-macro-redefined)