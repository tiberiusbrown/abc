cmake_minimum_required(VERSION 3.15)

project(abc)

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
    
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(GetGitRevisionDescription)
git_describe(ABC_VERSION)
message(STATUS "ABC_VERSION: ${ABC_VERSION}")
string(REGEX REPLACE "^v" "" ABC_VERSION_SHORT ${ABC_VERSION})
string(REGEX REPLACE "-.*" "" ABC_VERSION_SHORT ${ABC_VERSION_SHORT})
message(STATUS "ABC_VERSION_SHORT: ${ABC_VERSION_SHORT}")

set(EXE_TYPE)
if(MSVC)
    set(EXE_TYPE WIN32)
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
        /W4
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
endif()

if(EMSCRIPTEN)
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -O3")
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -flto")
    add_link_options("--closure 1")
    add_link_options("-s MODULARIZE=1")
    add_link_options("-flto")
    
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -fno-exceptions")
    #set(EXTRA_FLAGS "${EXTRA_FLAGS} -fno-rtti ")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${EXTRA_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_FLAGS}")
    add_link_options("-sALLOW_MEMORY_GROWTH")
    add_link_options("-sINITIAL_MEMORY=24MB")
    add_link_options("-sEXPORTED_FUNCTIONS=_main,_malloc,_free")
    add_link_options("-sEXPORTED_RUNTIME_METHODS=ccall")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['$autoResumeAudioContext','$dynCall']")
endif()

set(EM_FILE_BROWSER_SOURCES)
if(EMSCRIPTEN)
    set(EM_FILE_BROWSER_SOURCES deps/emscripten-browser-file/emscripten_browser_file.h)
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
    src/ards_compiler_codegen_expr.cpp
    src/ards_compiler_parse.cpp
    src/ards_compiler_peephole.cpp
    src/ards_compiler_progdata.cpp
    src/ards_compiler_sprites.cpp
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
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/Ardens/src/rapidjson/include"
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

add_executable(abc ${EXE_TYPE}
    src/font.cpp
    src/font_icons.cpp
    src/font_icons.hpp
    src/ide_common.cpp
    src/ide_common.hpp
    src/ide_compile.cpp
    src/ide_editor.cpp
    src/ide_export.cpp
    src/ide_hexdata.cpp
    src/ide_import.cpp
    src/ide_main.cpp
    src/ide_new_project.cpp
    src/ide_player.cpp
    src/ide_project.cpp
    src/ide_project_info.cpp
    src/ide_texture.cpp

    "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    
    ${EM_FILE_BROWSER_SOURCES}
    ${IMGUI_SOURCES}
    ${SOKOL_SOURCES}
    ${TEXTEDIT_SOURCES}
    )
target_link_libraries(abc PRIVATE ardenslib ards_compiler)
target_include_directories(abc PRIVATE
    deps/sokol
    deps/imgui
    deps/ImGuiColorTextEdit
    deps/emscripten-browser-file
    "${GENHEADER_DIR}"
    )

if((WIN32 OR APPLE OR UNIX) AND NOT EMSCRIPTEN)
    add_subdirectory(deps/nativefiledialog-extended)
    target_link_libraries(abc PRIVATE nfd)
endif()

add_executable(abc_benchmarks benchmarks/abc_benchmarks.cpp)
target_link_libraries(abc_benchmarks PRIVATE ardenslib ards_compiler)
target_include_directories(abc_benchmarks PRIVATE "${GENHEADER_DIR}")
target_compile_definitions(abc_benchmarks PRIVATE
    -DBENCHMARKS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    )

add_executable(abc_tests tests/abc_tests.cpp)
target_link_libraries(abc_tests PRIVATE ardenslib ards_compiler)
target_include_directories(abc_tests PRIVATE "${GENHEADER_DIR}")
target_compile_definitions(abc_tests PRIVATE
    -DTESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/tests"
    )

set_source_files_properties(
    src/ide_common.cpp
    PROPERTIES
        COMPILE_OPTIONS "-DABC_VERSION=\"${ABC_VERSION}\""
    )
