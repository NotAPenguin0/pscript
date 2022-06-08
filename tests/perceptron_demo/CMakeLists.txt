add_executable(pscript-demo main.cpp)

target_link_libraries(pscript-demo PRIVATE plib)
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