@echo off
setlocal enabledelayedexpansion
cls

set "CONFIG_FILE=config.ini"

:: Check if config.ini exists; if not, jump to setup configuration phase
if not exist "%CONFIG_FILE%" goto INITIAL_SETUP

:: Read paths directly out of the config.ini file, stripping whitespace
for /f "tokens=1,2 delims==" %%A in ('type "%CONFIG_FILE%" ^| findstr /v "\["') do (
    set "key=%%A"
    set "val=%%B"
    :: Strip leading/trailing spaces from key/value names
    for /f "tokens=*" %%X in ("!key!") do set "key=%%X"
    for /f "tokens=*" %%X in ("!val!") do set "val=%%X"
    
    :: Advanced Trimming Loop: Removes trailing spaces from paths safely
    set "clean_val=!val!"
    for /l %%I in (1,1,4) do (
        if "!clean_val:~-1!"==" " set "clean_val=!clean_val:~0,-1!"
    )
    if "!key!"=="W64_BIN_PATH" set "W64_BIN_PATH=!clean_val!"
    if "!key!"=="W32_BIN_PATH" set "W32_BIN_PATH=!clean_val!"
)
goto MENU

:INITIAL_SETUP
echo ========================================================
echo  First-Time Setup: PyCity Path Configuration Wizard
echo ========================================================
echo  Please paste or type the absolute paths directly to 
echo  your "bin" folders (e.g., C:\w64devkit\bin).
echo  Do not wrap them in quotation marks here.
echo ========================================================
echo.

set /p "INPUT_64=Enter path to your 64-bit w64devkit\bin folder: "
set /p "INPUT_32=Enter path to your 32-bit w32devkit\bin folder: "

:: Strip any accidental quotes input by the user
set "INPUT_64=%INPUT_64:"=%"
set "INPUT_32=%INPUT_32:"=%"

:: Write structured INI configuration format with headers
echo [Paths] > "%CONFIG_FILE%"
echo W64_BIN_PATH=%INPUT_64% >> "%CONFIG_FILE%"
echo W32_BIN_PATH=%INPUT_32% >> "%CONFIG_FILE%"

set "W64_BIN_PATH=%INPUT_64%"
set "W32_BIN_PATH=%INPUT_32%"
echo.
echo [+] Bin paths successfully saved to %CONFIG_FILE%!
echo.
pause
cls
goto MENU

:MENU
echo ========================================================
echo  PyCity Native Windows Master Builder Suite
echo ========================================================
echo  Choose your compilation target architecture:
echo.
echo   1) x64 Production Build (64-bit Core Binary)
echo   2) x86 Legacy Compatibility Build (32-bit Core Binary)
echo   3) Build Both Architectures (Simultaneously)
echo   r) Reset Configuration Paths
echo   q) Exit Builder
echo ========================================================
echo.

set /p choice="Enter selection: "

if "%choice%"=="1" goto BUILD_64
if "%choice%"=="2" goto BUILD_32
if "%choice%"=="3" goto BUILD_BOTH
if "%choice%"=="r" goto RESET_CONFIG
if "%choice%"=="q" exit
goto MENU

:BUILD_64
echo.
echo [!] Initializing 64-bit Compilation Environment...
set "PATH=%W64_BIN_PATH%;%PATH%"
set "MAKE_CMD=make"
if exist "%W64_BIN_PATH%\mingw32-make.exe" set "MAKE_CMD=mingw32-make"
%MAKE_CMD% OUT=pycity-win64.exe RAYLIB_DIR=raylib64/src
goto END

:BUILD_32
echo.
echo [!] Initializing 32-bit Compilation Environment...
set "PATH=%W32_BIN_PATH%;%PATH%"
set "MAKE_CMD=make"
if exist "%W32_BIN_PATH%\mingw32-make.exe" set "MAKE_CMD=mingw32-make"
%MAKE_CMD% OUT=pycity-win32.exe RAYLIB_DIR=raylib32/src
goto END

:BUILD_BOTH
echo.
echo [!] Launching Parallel Dual-Architecture Build Threads...

:: Look for custom make binary naming conventions for both environments
set "MAKE_64=make"
if exist "%W64_BIN_PATH%\mingw32-make.exe" set "MAKE_64=mingw32-make"
set "MAKE_32=make"
if exist "%W32_BIN_PATH%\mingw32-make.exe" set "MAKE_32=mingw32-make"

:: Fire off both tasks at the exact same moment in background threads
start /b cmd /c "set PATH=%W64_BIN_PATH%;%%PATH%% && %MAKE_64% OUT=pycity-win64.exe RAYLIB_DIR=raylib64/src"
start /b cmd /c "set PATH=%W32_BIN_PATH%;%%PATH%% && %MAKE_32% OUT=pycity-win32.exe RAYLIB_DIR=raylib32/src"

:: Give the system a brief moment to write console outputs
timeout /t 2 >nul
goto END

:RESET_CONFIG
del "%CONFIG_FILE%"
echo.
echo [+] Configuration file deleted. Restarting setup wizard...
echo.
pause
cls
goto INITIAL_SETUP

:END
echo.
echo ========================================================
echo  Build process triggered!
echo ========================================================
pause
cls
goto MENU
