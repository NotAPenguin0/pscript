include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/NotAPenguin0/imgui
        GIT_TAG docking
)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
)

FetchContent_MakeAvailable(imgui glfw)

add_library(ps-glad STATIC external/glad330/src/glad.c)
target_include_directories(ps-glad PUBLIC external/glad330/include)

add_executable(pscript-demo main.cpp external/imgui_impl_opengl3.cpp external/imgui_impl_glfw.cpp)

target_link_libraries(pscript-demo PRIVATE plib imgui ps-glad glfw)
target_include_directories(pscript-demo PRIVATE {imgui_SOURCE_DIR}/include)
target_compile_options(pscript-demo PRIVATE -Wno-format -Wno-macro-redefined)

file(GLOB_RECURSE MODULE_FILES "../../modules/*")
add_custom_command(
        TARGET pscript-demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../modules/" ${CMAKE_CURRENT_BINARY_DIR}/pscript-modules
        DEPENDS ${MODULE_FILES}
        COMMENT "Copying plib modules - demo"
        VERBATIM
)

file(GLOB_RECURSE PS_SOURCES "ps/*")
add_custom_command(
        TARGET pscript-demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/ps" ${CMAKE_CURRENT_BINARY_DIR}/ps
        DEPENDS ${PS_SOURCES}
        COMMENT "Copying pscript sources - demo"
        VERBATIM
)
