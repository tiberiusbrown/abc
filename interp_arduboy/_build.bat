@echo off

cd /d %~dp0

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-fx . ^
    --output-dir "build_s2_ArduboyFX" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-mini . ^
    --output-dir "build_s2_ArduboyMini" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,based_on=promicro_alt,flashselect=rx . ^
    --output-dir "build_s2_homemade.promicro_alt.ssd1306.rx" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,display=sh1106 . ^
    --output-dir "build_s2_homemade.leonardo.sh1106.sda" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,display=sh1106,flashselect=rx . ^
    --output-dir "build_s2_homemade.leonardo.sh1106.rx" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-homemade:boot=cathy3k,display=sh1106,based_on=promicro_alt,flashselect=rx . ^
    --output-dir "build_s2_homemade.promicro_alt.sh1106.rx" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"





arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-fx . ^
    --output-dir "build_s3_ArduboyFX" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack -DABC_SHADES=3" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"

arduino-cli.exe compile ^
    -b arduboy-homemade:avr:arduboy-fx . ^
    --output-dir "build_s4_ArduboyFX" ^
    --build-property compiler.c.elf.extra_flags="-Wl,--relax -Wl,--section-start=.beforedata=0x800100 -Wl,--section-start=.data=0x80063e" ^
    --build-property compiler.c.extra_flags="-mtiny-stack -DABC_SHADES=4" ^
    --build-property compiler.cpp.extra_flags="{compiler.c.extra_flags}" ^
    --build-property compiler.elf2hex.extra_flags="-R .beforedata"
