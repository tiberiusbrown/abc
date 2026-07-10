When building or running any target, always use Debug configuration.

## Building

Build through CMake rather than invoking Ninja, MSBuild, or another backend directly:

```sh
cmake --build build --parallel
```

Preserve the existing CMake generator and compiler unless explicitly asked to change them.

On Windows, if the configured generator is Ninja or Ninja Multi-Config and the compiler is MSVC (`cl.exe`), ensure the matching MSVC developer environment is initialized before building. Match the Visual Studio/MSVC installation recorded in `build/CMakeCache.txt`; using a different MSVC version can cause ABI/linker errors. Missing setup commonly causes errors for headers such as `vector`, `stdio.h`, `stdint.h`, or `windows.h`.

When needed, locate Visual Studio with `vswhere.exe`, call `vcvars64.bat` or `VsDevCmd.bat`, and run `cmake --build` from that same shell. Do not hard-code Visual Studio, MSVC, or Windows SDK versions, and do not add toolchain header paths to `CMakeLists.txt`.

Do not perform MSVC environment setup on non-Windows systems, for non-MSVC compilers, or for Visual Studio/MSBuild generator builds unless required.
