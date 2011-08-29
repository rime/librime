@echo off
rem rime build script for mingw toolchain.
rem see HowToRimeWithTheCode wiki for explanations.
rem 
rem 2011-04-17 <chen.sst@gmail.com>

set BACK=%CD%

if exist env.bat call env.bat

set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%DEV_PATH%;%OLD_PATH%
path
echo.

if not defined RIME_ROOT set RIME_ROOT=%CD%
echo RIME_ROOT=%RIME_ROOT%
echo.

echo BOOST_ROOT=%BOOST_ROOT%
echo.

echo TODO: populate this folder with third-party library header files.
set CMAKE_INCLUDE_PATH=%RIME_ROOT%\thirdparty\include
echo CMAKE_INCLUDE_PATH=%CMAKE_INCLUDE_PATH%

echo.

echo TODO: populate this folder with third-party libraries.
set CMAKE_LIBRARY_PATH=%RIME_ROOT%\thirdparty\lib
echo CMAKE_LIBRARY_PATH=%CMAKE_LIBRARY_PATH%

echo.

rem TODO: select a cmake generator
rem set CMAKE_GENERATOR="MinGW Makefiles"
set CMAKE_GENERATOR="Eclipse CDT4 - MinGW Makefiles"

set BUILD_DIR=%RIME_ROOT%\build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

cd %BUILD_DIR%
cmake -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=Debug %RIME_ROOT%
if %ERRORLEVEL% NEQ 0 goto ERROR
make
if %ERRORLEVEL% NEQ 0 goto ERROR
make test
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo done.
echo.
goto EXIT

:ERROR
echo.
echo error building la rime.
echo.

:EXIT
set PATH=%OLD_PATH%
cd %RIME_ROOT%
rem pause
