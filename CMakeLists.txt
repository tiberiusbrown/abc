cmake_minimum_required(VERSION 3.15)

project(abc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/cmake-bin2h/bin2h.cmake)
set(GENHEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin2h_headers")
file(MAKE_DIRECTORY "${GENHEADER_DIR}")
bin2h(
    SOURCE_FILE   interp_arduboy/build/interp_arduboy.ino.hex
    HEADER_FILE   "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    VARIABLE_NAME VM_HEX_ARDUBOYFX
    )
bin2h(
    SOURCE_FILE   interp_arduboy/build_mini/interp_arduboy.ino.hex
    HEADER_FILE   "${GENHEADER_DIR}/vm_hex_arduboymini.hpp"
    VARIABLE_NAME VM_HEX_ARDUBOYMINI
    )
    
bin2h(
    SOURCE_FILE   examples/basic/main.abc
    HEADER_FILE   "${GENHEADER_DIR}/basic_main.hpp"
    VARIABLE_NAME BASIC_MAIN
    )

file(GLOB ABC_FONTS "fonts/*.ttf")
set(ABC_FONT_HEADERS)
set(ABC_FONT_MASTER "${GENHEADER_DIR}/all_fonts.hpp")
list(APPEND ABC_FONT_HEADERS "${ABC_FONT_MASTER}")
file(WRITE "${ABC_FONT_MASTER}" "#pragma once\n")
file(APPEND "${ABC_FONT_MASTER}" "#include <stdint.h>\n")
file(APPEND "${ABC_FONT_MASTER}" "struct font_info_t {\n")
file(APPEND "${ABC_FONT_MASTER}" "    char const* name;\n")
file(APPEND "${ABC_FONT_MASTER}" "    size_t pixels;\n")
file(APPEND "${ABC_FONT_MASTER}" "    unsigned char const* data;\n")
file(APPEND "${ABC_FONT_MASTER}" "    size_t size;\n};\n")
foreach(F ${ABC_FONTS})
    get_filename_component(FB "${F}" NAME_WE)
    string(REGEX REPLACE "_x[0-9]+$" "" FB "${FB}")
    file(APPEND "${ABC_FONT_MASTER}" "#include \"${FB}.hpp\"\n")
endforeach()
file(APPEND "${ABC_FONT_MASTER}" "static font_info_t const ALL_FONTS[] = {\n")
foreach(F ${ABC_FONTS})
    get_filename_component(FB "${F}" NAME_WE)
    string(REGEX MATCH "[0-9]+$" FN "${FB}")
    string(REGEX REPLACE "_x[0-9]+$" "" FB "${FB}")
    string(TOUPPER "${FB}" FBU)
    bin2h(
        SOURCE_FILE   "${F}"
        HEADER_FILE   "${GENHEADER_DIR}/${FB}.hpp"
        VARIABLE_NAME "${FBU}"
        )
    file(APPEND "${ABC_FONT_MASTER}" "    { \"${FBU}\", ${FN}, ${FBU}, sizeof(${FBU}) },\n")
endforeach()
file(APPEND "${ABC_FONT_MASTER}" "};\n")

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
    add_link_options("-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_postsyncfs")
    add_link_options("-sEXPORTED_RUNTIME_METHODS=ccall,FS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['$autoResumeAudioContext','$dynCall']")
endif()

set(STATIC_CRT      ON  CACHE BOOL "" FORCE)
add_subdirectory(deps/midifile EXCLUDE_FROM_ALL)

set(ARDENS_LLVM     OFF CACHE BOOL "" FORCE)
set(ARDENS_DEBUGGER OFF CACHE BOOL "" FORCE)
set(ARDENS_PLAYER   OFF CACHE BOOL "" FORCE)
set(ARDENS_LIB      ON  CACHE BOOL "" FORCE)
set(ARDENS_LIBRETRO OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/Ardens)

set(EM_FILE_BROWSER_SOURCES)
if(EMSCRIPTEN)
    set(EM_FILE_BROWSER_SOURCES deps/emscripten-browser-file/emscripten_browser_file.h)
endif()

add_library(ards_compiler STATIC
    .editorconfig
    interp_general/abc_instr.hpp
    src/ards_assembler.cpp
    src/ards_assembler.hpp
    src/ards_compiler.cpp
    src/ards_compiler.hpp
    src/ards_compiler_codegen.cpp
    src/ards_compiler_codegen_expr.cpp
    src/ards_compiler_font.cpp
    src/ards_compiler_parse.cpp
    src/ards_compiler_peephole.cpp
    src/ards_compiler_progdata.cpp
    src/ards_compiler_sprites.cpp
    src/ards_compiler_tones.cpp
    src/ards_compiler_transforms.cpp
    src/ards_compiler_type.cpp
    src/ards_compiler_write.cpp
    src/ards_error.hpp
    src/ards_sysfuncs.cpp
    deps/peglib/peglib.h
    )
target_include_directories(ards_compiler PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/interp_general"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/peglib"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/Ardens/deps/rapidjson/include"
    )
target_include_directories(ards_compiler PRIVATE "${GENHEADER_DIR}")
target_include_directories(ards_compiler PRIVATE deps/midifile/include)
target_link_libraries(ards_compiler PRIVATE fmt midifile)

add_executable(abcc
    src/abcc.cpp
    src/ide_export_arduboy.cpp
    )
target_link_libraries(abcc ards_compiler ardenslib)
target_include_directories(abcc PRIVATE
    "${GENHEADER_DIR}"
    deps/argparse/include
    )

file(GLOB IMGUI_SOURCES deps/imgui/*.h deps/imgui/*.cpp)
file(GLOB SOKOL_SOURCES deps/sokol/*.h)
file(GLOB TEXTEDIT_SOURCES deps/ImGuiColorTextEdit/*.h deps/ImGuiColorTextEdit/*.cpp)

add_executable(abc ${EXE_TYPE}
    src/ctrl2o.hpp
    src/font.cpp
    src/font_icons.cpp
    src/font_icons.hpp
    src/ide_common.cpp
    src/ide_common.hpp
    src/ide_compile.cpp
    src/ide_editor.cpp
    src/ide_export.cpp
    src/ide_export_arduboy.cpp
    src/ide_import.cpp
    src/ide_main.cpp
    src/ide_player.cpp
    src/ide_project.cpp
    src/ide_sys.cpp
    src/ide_texture.cpp
    src/ide_welcome.cpp

    "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    "${GENHEADER_DIR}/basic_main.hpp"
    
    ${EM_FILE_BROWSER_SOURCES}
    ${IMGUI_SOURCES}
    ${SOKOL_SOURCES}
    ${TEXTEDIT_SOURCES}
    )
target_link_libraries(abc PRIVATE ardenslib ards_compiler)
if(WIN32)
    target_link_libraries(abc PRIVATE d3d11)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(abc PRIVATE X11 Xi Xcursor GL asound dl m pthread)
endif()
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

add_executable(abc_integration
    benchmarks/abc_benchmarks.cpp
    docs/abc_docs.cpp
    tests/abc_tests.cpp
    src/abc_integration.cpp
    )
target_link_libraries(abc_integration PRIVATE ardenslib ards_compiler)
target_include_directories(abc_integration PRIVATE "${GENHEADER_DIR}")
target_compile_definitions(abc_integration PRIVATE
    -DVMHEX_FILE="${CMAKE_CURRENT_SOURCE_DIR}/interp_arduboy/build/interp_arduboy.ino.hex"
    -DBENCHMARKS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    -DDOCS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/docs"
    -DTESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/tests"
    -DPLATFORMER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples/platformer"
    )
    
set_source_files_properties(
    src/ide_common.cpp
    src/abcc.cpp
    PROPERTIES
        COMPILE_OPTIONS "-DABC_VERSION=\"${ABC_VERSION}\""
    )

if(EMSCRIPTEN)
    target_link_options(abc PRIVATE "-lidbfs.js")
endif()
