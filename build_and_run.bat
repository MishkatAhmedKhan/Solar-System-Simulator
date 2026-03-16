@echo off
rem =========================================================================
rem SOLAR SYSTEM SIMULATOR
rem Build and Run script for Windows (MSYS2 / UCRT64)
rem =========================================================================

echo -------------------------------------------------------------------------
echo Building Solar System Simulator...
echo -------------------------------------------------------------------------

rem Add MSYS2 UCRT64 to PATH just for this script's execution
set PATH=C:\msys64\ucrt64\bin;%PATH%

rem The compiler command
g++.exe -std=gnu++17 ^
    src/main.cpp src/physics.cpp src/scene.cpp src/camera.cpp src/glad.c src/imgui/*.cpp ^
    -Iinclude -Isrc -Iinclude/imgui ^
    -Llib ^
    -lglfw3dll -lglew32 -lopengl32 -lgdi32 ^
    -o bin/app.exe

if %errorlevel% neq 0 (
    echo.
    echo -------------------------------------------------------------------------
    echo BUILD FAILED!
    echo -------------------------------------------------------------------------
    pause
    exit /b %errorlevel%
)

echo.
echo -------------------------------------------------------------------------
echo BUILD SUCCESSFUL! Launching...
echo -------------------------------------------------------------------------
echo.

rem Navigate to the bin directory and run to ensure working directory is correct for any assets
cd bin
start app.exe
cd ..
