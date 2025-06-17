@REM launch_windows_cmd.bat
@REM Launches Windows CMD.EXE with appropriate PATH
@REM Bryan E. Topp <betopp@betopp.com> 2025

@title PVMK SDK Command Prompt
@echo Putting the SDK at the end of the PATH...
@set PATH=%PATH%;%~dp0\bin
@echo(

@echo SDK built from Git version:
@cmd /c pvmk-sdkversion -vc
@echo(

@echo Welcome to the PVMK SDK for Windows.
@echo Utilities shipped with this SDK all begin with "pvmk-".
@echo Check out the examples and build them with "pvmk-make".
@echo Make something fun!
@echo(


@cmd
