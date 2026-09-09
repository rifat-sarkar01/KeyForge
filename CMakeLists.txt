cmake_minimum_required(VERSION 3.20)
project(KeyForge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/ui/main_window.cpp
    src/ui/key_picker_dialog.cpp
    src/ui/renderer_d2d.cpp
    src/ui/theme.cpp
    src/layout/layout_loader.cpp
    src/engine/remap_table.cpp
    src/backend_hook/keyboard_hook.cpp
    src/backend_hook/raw_input_monitor.cpp
    src/backend_registry/scancode_map_writer.cpp
    src/profiles/profile_store.cpp
    src/tray/tray_icon.cpp
    src/keyforge.rc
)

add_executable(KeyForge WIN32 ${SOURCES})

target_include_directories(KeyForge PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(KeyForge PRIVATE
    d2d1
    dwrite
    user32
    gdi32
    shell32
    advapi32
    comctl32
    ole32
)

set_property(TARGET KeyForge PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(KeyForge PRIVATE /W4 /WX- /utf-8)
    target_link_options(KeyForge PRIVATE $<$<CONFIG:Release>:/INCREMENTAL:NO>)
    target_compile_definitions(KeyForge PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

enable_testing()

# Each test file defines its own main(), so each gets its own executable -
# combining them into a single target fails to link ("multiple definition
# of main").
function(add_keyforge_test test_name)
    add_executable(${test_name} ${ARGN})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/src
    )
    target_compile_definitions(${test_name} PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_keyforge_test(test_scancode_map_builder
    tests/test_scancode_map_builder.cpp
    src/backend_registry/scancode_map_writer.cpp
)

add_keyforge_test(test_layout_loader
    tests/test_layout_loader.cpp
    src/layout/layout_loader.cpp
)

add_keyforge_test(test_remap_table
    tests/test_remap_table.cpp
    src/engine/remap_table.cpp
    src/layout/layout_loader.cpp
)
