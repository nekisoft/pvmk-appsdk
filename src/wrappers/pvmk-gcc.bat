@REM Toolchain wrapper for PVMK SDK
@REM Bryan E. Topp <betopp@betopp.com> 2025
@call %~dp0\pvmk-vars.bat
%PVMKBINS%\pvmk-gcc -B%PVMKBINS% -B%PVMKBINS%\gcc\armv5te-pvmk-eabi\15.1.0 --picolibc-prefix=%SYSROOT% --specs=%SYSROOT%\armv5te-pvmk-eabi\lib\pvmk.specs %*
