:: Win$hit BAT file to help make Licar under that horrible OS. It probably won't
:: work right away, but can hint on how to compile it. MinGW is used as a compiler.
:: It's possible to use this under Wine to not have to touch the horrible OS.
echo making Licar
bin/g++.exe -std=c99 -O3 -DSDL_MAIN_HANDLED -ISDL2-2.30.9/x86_64-w64-mingw32/include/ -LSDL2-2.30.9/x86_64-w64-mingw32/lib/ -o licar.exe frontend_sdl.c -lSDL2
echo done
