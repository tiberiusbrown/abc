cmake_minimum_required(VERSION 3.21)

project(abc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/cmake-bin2h/bin2h.cmake)
set(GENHEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin2h_headers")
file(MAKE_DIRECTORY "${GENHEADER_DIR}")

file(GLOB INTERP_BUILDS "${CMAKE_CURRENT_SOURCE_DIR}/interp_arduboy/build_*")
set(BUILDZIP "${CMAKE_CURRENT_BINARY_DIR}/interp_builds.zip")
set(BUILDZIPDIR "${CMAKE_CURRENT_BINARY_DIR}/interp_builds")
file(REMOVE_RECURSE "${BUILDZIPDIR}")
file(MAKE_DIRECTORY "${BUILDZIPDIR}")
foreach(B ${INTERP_BUILDS})
    get_filename_component(BB "${B}" NAME)
    file(COPY_FILE "${B}/interp_arduboy.ino.hex" "${BUILDZIPDIR}/${BB}.hex")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar cf "${BUILDZIP}" --format=zip -- "interp_builds"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
endforeach()
bin2h(
    SOURCE_FILE   "${BUILDZIP}"
    HEADER_FILE   "${GENHEADER_DIR}/interp_builds.hpp"
    VARIABLE_NAME INTERP_BUILDS_ZIP
    )

bin2h(
    SOURCE_FILE   interp_arduboy/build_s2_ArduboyFX/interp_arduboy.ino.hex
    HEADER_FILE   "${GENHEADER_DIR}/vm_hex_arduboyfx.hpp"
    VARIABLE_NAME VM_HEX_ARDUBOYFX
    )
    
bin2h(
    SOURCE_FILE   examples/basic/main.abc
    HEADER_FILE   "${GENHEADER_DIR}/basic_main.hpp"
    VARIABLE_NAME BASIC_MAIN
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
        /D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        # MSVC's MASM tool treats this as an invalid option, so keep it off ASM_MASM.
        $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
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
    add_link_options("-sSUPPORT_LONGJMP=1")
    add_link_options("-sALLOW_MEMORY_GROWTH")
    add_link_options("-sINITIAL_MEMORY=24MB")
    add_link_options("-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_postsyncfs")
    add_link_options("-sEXPORTED_RUNTIME_METHODS=ccall,FS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['$autoResumeAudioContext','$dynCall']")
endif()

set(ARDENS_LLVM     OFF CACHE BOOL "" FORCE)
set(ARDENS_DEBUGGER OFF CACHE BOOL "" FORCE)
set(ARDENS_PLAYER   OFF CACHE BOOL "" FORCE)
set(ARDENS_LIB      ON  CACHE BOOL "" FORCE)
set(ARDENS_LIBRETRO OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/Ardens EXCLUDE_FROM_ALL)

if(NOT EMSCRIPTEN)
    set(LLVM_BUILD_LLVM_DYLIB              ON  CACHE BOOL   "" FORCE)
    set(LLVM_BUILD_TESTS                   OFF CACHE BOOL   "" FORCE)
    set(LLVM_BUILD_TOOLS                   OFF CACHE BOOL   "" FORCE)
    set(LLVM_BUILD_UTILS                   OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_TESTS                 OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_TOOLS                 ON  CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_UTILS                 OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_BENCHMARKS            OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_DOCS                  OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_EXAMPLES              OFF CACHE BOOL   "" FORCE)
    set(LLVM_INCLUDE_RUNTIMES              OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_BINDINGS               OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_LIBEDIT                OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_LIBPFM                 OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_OCAMLDOC               OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_TIMESTAMPS             OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_TERMINFO               OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_ZLIB                   OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_ZSTD                   OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_LIBXML2                OFF CACHE BOOL   "" FORCE)
    set(LLVM_ENABLE_PROJECTS           "clang;lld" CACHE STRING "" FORCE)
    set(_ENABLE_LLVM_NATIVE_BUILD          OFF CACHE BOOL   "" FORCE)
    set(LLVM_TARGETS_TO_BUILD              ";" CACHE STRING "" FORCE)
    set(LLVM_EXPERIMENTAL_TARGETS_TO_BUILD "ABC" CACHE STRING "" FORCE)

    add_subdirectory(deps/llvm-project-abc/llvm EXCLUDE_FROM_ALL)
    add_custom_target(abc-llvm-tools ALL DEPENDS
        clang lld llvm-mc llvm-objdump llvm-readobj llvm-size
        abc-as abc-objdump abc-readobj abc-size
        )
endif()

add_library(abc_interp STATIC
    .editorconfig
    interp_generic/abc_interp.h
    interp_generic/abc_interp.c
    )
if(MSVC)
    target_compile_options(abc_interp PRIVATE /Za)
endif()
target_include_directories(abc_interp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/interp_generic"
    )

set(SDL_FORCE_STATIC_VCRT ON CACHE BOOL "" FORCE)
add_subdirectory(deps/SDL)
add_executable(abc_interpreter_sdl2 ${EXE_TYPE}
    .editorconfig
    interp_generic/abc_interpreter_sdl2.c
    )
target_link_libraries(abc_interpreter_sdl2 abc_interp SDL2main SDL2-static)

add_executable(abc_interpreter_sokol ${EXE_TYPE}
    .editorconfig
    interp_generic/abc_interpreter_sokol.c
    )
target_link_libraries(abc_interpreter_sokol PRIVATE abc_interp)
if(WIN32)
    target_link_libraries(abc_interpreter_sokol PRIVATE d3d11)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(abc_interpreter_sokol PRIVATE X11 Xi Xcursor GL asound dl m pthread)
endif()

if(NOT EMSCRIPTEN)
    set(ABC_CLANG_FIXTURE_ASM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/abc_clang/asm")
    set(ABC_CLANG_TEST_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/abc_clang_tests")

    function(add_abc_clang_fixture name)
        set(src "${CMAKE_CURRENT_SOURCE_DIR}/tests/abc_clang/${name}.c")
        set(asm_out "${ABC_CLANG_FIXTURE_ASM_DIR}/${name}.s")
        set(obj_out "${ABC_CLANG_TEST_WORK_DIR}/${name}.obj")
        set(bin_out "${ABC_CLANG_TEST_WORK_DIR}/${name}.bin")

        add_custom_command(
            OUTPUT "${asm_out}" "${obj_out}" "${bin_out}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${ABC_CLANG_FIXTURE_ASM_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${ABC_CLANG_TEST_WORK_DIR}"
            COMMAND $<TARGET_FILE:clang> --target=abc -Os -ffreestanding -fno-builtin -nostdlib -S "${src}" -o "${asm_out}"
            COMMAND $<TARGET_FILE:clang> --target=abc -Os -ffreestanding -fno-builtin -nostdlib -c "${src}" -o "${obj_out}"
            COMMAND $<TARGET_FILE_DIR:lld>/abc-ld${CMAKE_EXECUTABLE_SUFFIX} "${obj_out}" -o "${bin_out}"
            DEPENDS "${src}" clang
            VERBATIM
            COMMENT "Generating abc_clang fixture ${name}"
        )

        set_property(GLOBAL APPEND PROPERTY ABC_CLANG_FIXTURE_OUTPUTS
            "${asm_out}"
            "${obj_out}"
            "${bin_out}"
            )
    endfunction()

    function(add_abc_clang_asm_fixture name)
        set(src "${CMAKE_CURRENT_SOURCE_DIR}/tests/abc_clang/${name}.c")
        set(asm_out "${ABC_CLANG_FIXTURE_ASM_DIR}/${name}.s")

        add_custom_command(
            OUTPUT "${asm_out}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${ABC_CLANG_FIXTURE_ASM_DIR}"
            COMMAND $<TARGET_FILE:clang> --target=abc -Os -ffreestanding -fno-builtin -nostdlib -S "${src}" -o "${asm_out}"
            DEPENDS "${src}" clang
            VERBATIM
            COMMENT "Generating abc_clang assembly ${name}"
        )

        set_property(GLOBAL APPEND PROPERTY ABC_CLANG_FIXTURE_OUTPUTS "${asm_out}")
    endfunction()

    add_abc_clang_fixture(printf_static)
    add_abc_clang_fixture(printf_char)
    add_abc_clang_fixture(printf_ram_string)
    add_abc_clang_fixture(printf_prog_string)
    add_abc_clang_fixture(printf_mixed_strings)
    add_abc_clang_fixture(printf_integers)
    add_abc_clang_asm_fixture(basic)
    add_abc_clang_asm_fixture(basic_void)
    add_abc_clang_asm_fixture(basic_memset)
    add_abc_clang_asm_fixture(basic_memset_used)

    get_property(ABC_CLANG_FIXTURE_OUTPUTS GLOBAL PROPERTY ABC_CLANG_FIXTURE_OUTPUTS)
    add_custom_target(abc_clang_fixtures DEPENDS ${ABC_CLANG_FIXTURE_OUTPUTS})
    add_dependencies(abc_clang_fixtures abc-llvm-tools)

    add_executable(abc_clang_tests
        .editorconfig
        tests/abc_clang_tests.cpp
        )
    add_dependencies(abc_clang_tests abc-llvm-tools)
    add_dependencies(abc_clang_tests abc_clang_fixtures)
    target_include_directories(abc_clang_tests PRIVATE
        "${GENHEADER_DIR}"
        )
    target_link_libraries(abc_clang_tests PRIVATE
        abc_interp
        ardenslib
        )
    target_compile_definitions(abc_clang_tests PRIVATE
        ABC_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/abc_clang"
        ABC_TEST_WORK_DIR="${ABC_CLANG_TEST_WORK_DIR}"
        )
    add_custom_target(check-abc-clang
        COMMAND abc_clang_tests
        DEPENDS abc_clang_tests
        USES_TERMINAL
        )
endif()
