@echo off

cd /d %~dp0

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-fx . ^
    --output-dir "build" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063d" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-mini . ^
    --output-dir "build_mini" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063d" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,based_on=promicro_alt,flashselect=rx . ^
    --output-dir "build_homemade_alt" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063d" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,display=sh1106 . ^
    --output-dir "build_homemade_sh1106" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063d" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,display=sh1106,based_on=promicro_alt,flashselect=rx . ^
    --output-dir "build_homemade_sh1106_alt" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063d" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"
