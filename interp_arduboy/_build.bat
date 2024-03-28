@echo off

cd /d %~dp0

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-fx . ^
    --output-dir "build" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x800639" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-mini . ^
    --output-dir "build_mini" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x800639" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"
