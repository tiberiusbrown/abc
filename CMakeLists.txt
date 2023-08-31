cmake_minimum_required(VERSION 3.15)

project(ardscript)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/cmake-bin2h/bin2h.cmake)
set(GENHEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin2h_headers")
file(MAKE_DIRECTORY "${GENHEADER_DIR}")
bin2h(
    SOURCE_FILE   ards_vm/ards_vm.ino-arduboy-fx.hex
    HEADER_FILE   "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    VARIABLE_NAME VM_HEX_ARDUBOYFX
    )
    
set(EXE_TYPE)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    #set(EXE_TYPE WIN32)
    add_link_options(
        /DEBUG
        /INCREMENTAL:NO                     # Disable incremental linking.
        $<$<NOT:$<CONFIG:DEBUG>>:/OPT:REF>  # Remove unreferenced functions and data.
        $<$<NOT:$<CONFIG:DEBUG>>:/OPT:ICF>  # Identical COMDAT folding.
        $<$<NOT:$<CONFIG:DEBUG>>:/LTCG>     # Link-Time Code Generation.
        $<$<CONFIG:DEBUG>:/OPT:NOREF>       # No unreferenced data elimination.
        $<$<CONFIG:DEBUG>:/OPT:NOICF>       # No Identical COMDAT folding.
    )
    add_compile_options(
        /Zc:__cplusplus
        /utf-8
        /D_CRT_SECURE_NO_DEPRECATE
        $<$<NOT:$<CONFIG:DEBUG>>:/GL>
        $<$<NOT:$<CONFIG:DEBUG>>:/Gy>
        $<$<NOT:$<CONFIG:DEBUG>>:/Gw>
        $<$<NOT:$<CONFIG:DEBUG>>:/GF>
        $<$<NOT:$<CONFIG:DEBUG>>:/Oy>
        $<$<NOT:$<CONFIG:DEBUG>>:/Ot>
        $<$<NOT:$<CONFIG:DEBUG>>:/Ox>
    )
else()
    add_compile_options(-Wall -Wextra)
endif()

add_library(ards_assembler STATIC
    .editorconfig
    ards_vm/ards_instr.hpp
    src/ards_assembler.cpp
    src/ards_assembler.hpp
    src/ards_error.hpp
    )
target_include_directories(ards_assembler PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/ards_vm"
    )

add_library(ards_compiler STATIC
    .editorconfig
    ards_vm/ards_instr.hpp
    src/ards_assembler.cpp
    src/ards_assembler.hpp
    src/ards_compiler.cpp
    src/ards_compiler.hpp
    src/ards_compiler_codegen.cpp
    src/ards_compiler_parse.cpp
    src/ards_compiler_peephole.cpp
    src/ards_compiler_transforms.cpp
    src/ards_compiler_type.cpp
    src/ards_compiler_write.cpp
    src/ards_error.hpp
    deps/peglib/peglib.h
    )
target_include_directories(ards_compiler PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/ards_vm"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/peglib"
    )

add_executable(ards_as src/ards_as.cpp)
target_link_libraries(ards_as ards_assembler)

add_executable(ards_c src/ards_c.cpp)
target_link_libraries(ards_c ards_compiler)

set(ARDENS_LLVM     OFF CACHE BOOL "" FORCE)
set(ARDENS_DEBUGGER OFF CACHE BOOL "" FORCE)
set(ARDENS_PLAYER   OFF CACHE BOOL "" FORCE)
set(ARDENS_LIB      ON  CACHE BOOL "" FORCE)
set(ARDENS_LIBRETRO OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/Ardens)

file(GLOB IMGUI_SOURCES deps/imgui/*.h deps/imgui/*.cpp)
file(GLOB SOKOL_SOURCES deps/sokol/*.h)
file(GLOB TEXTEDIT_SOURCES deps/ImGuiColorTextEdit/*.h deps/ImGuiColorTextEdit/*.cpp)

add_executable(abc_ide ${EXE_TYPE}
    src/font.cpp
    src/font_icons.cpp
    src/font_icons.hpp
    src/ide_common.cpp
    src/ide_common.hpp
    src/ide_compile.cpp
    src/ide_editor.cpp
    src/ide_export.cpp
    src/ide_hexdata.cpp
    src/ide_main.cpp
    src/ide_new_project.cpp
    src/ide_player.cpp
    src/ide_project.cpp
    src/ide_project_info.cpp
    src/ide_texture.cpp

    "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    
    ${IMGUI_SOURCES}
    ${SOKOL_SOURCES}
    ${TEXTEDIT_SOURCES}
    )
target_link_libraries(abc_ide PRIVATE ardenslib ards_compiler)
target_include_directories(abc_ide PRIVATE
    deps/sokol
    deps/imgui
    deps/ImGuiColorTextEdit
    "${GENHEADER_DIR}"
    )

if(WIN32 OR APPLE OR UNIX)
    add_subdirectory(deps/nativefiledialog-extended)
    target_link_libraries(abc_ide PRIVATE nfd)
endif()

add_executable(abc_benchmarks benchmarks/abc_benchmarks.cpp)
target_link_libraries(abc_benchmarks PRIVATE ardenslib ards_compiler)
target_include_directories(abc_benchmarks PRIVATE "${GENHEADER_DIR}")
target_compile_definitions(abc_benchmarks PRIVATE
    -DBENCHMARKS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    )
