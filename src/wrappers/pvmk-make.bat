@REM Toolchain wrapper for PVMK SDK
@REM Bryan E. Topp <betopp@betopp.com> 2025
@call %~dp0\pvmk-vars.bat
@%PVMKBINS%\%~n0 %*
