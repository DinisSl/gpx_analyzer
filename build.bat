@echo off
@REM :: Check for admin privileges
@REM net session >nul 2>&1
@REM if %errorLevel% neq 0 (
@REM     echo Requesting administrative privileges...
@REM     powershell -Command "Start-Process '%~f0' -Verb runAs"
@REM     exit /b
@REM )
@REM :main
cls
echo ===== Compiling gpx_analyzer.c =====
gcc gpx_analyzer.c ^
    -I"C:\Program Files\GeographicLib" ^
    -I"C:\Path\To\Expat\include" ^
    "C:\Program Files\GeographicLib\geodesic.o" ^
    -L"C:\Program Files\GeographicLib" ^
    -L"C:\Path\To\Expat\lib" ^
    -lgeographic ^
    -lexpat ^
    -o gpx_analyzer.exe


if errorlevel 1 (
    echo.
    echo *** Compilation FAILED! ***
    pause
    exit /b 1
)

echo.
echo *** Compilation SUCCEEDED ***
echo ===================================
echo.

rem — prompt for your two arguments —
set /p arg1="Enter first argument: "
set /p arg2="Enter second argument: "
echo.

echo Running: gpx_analyzer.exe %arg1% %arg2%
echo ===================================
echo.
gpx_analyzer.exe %arg1% %arg2%
echo.
echo ===================================

rem — clean up the executable so it's recompiled next time —
del gpx_analyzer.exe

echo.
echo Program finished.

:choice
echo Press [Enter] to rerun or [ESC] to exit...

:: Use PowerShell to detect Enter (13) or ESC (27)
for /f %%a in ('powershell -command "$key = [System.Console]::ReadKey($true).Key; if ($key -eq 'Enter') { 13 } elseif ($key -eq 'Escape') { 27 }"') do (
    set "keycode=%%a"
)

if "%keycode%"=="13" goto main
if "%keycode%"=="27" goto end

:end
echo Exiting...
@REM gcc gpx_analyzer.c -IC:\Users\dinis\OneDrive\Documentos\VSCode\UserDinis\running\GegraphicLib C:\Users\dinis\OneDrive\Documentos\VSCode\UserDinis\running\GegraphicLib\geodesic.o -LC:\Users\dinis\OneDrive\Documentos\VSCode\UserDinis\running\GegraphicLib -lgeographic -o gpx_analyzer.exe
