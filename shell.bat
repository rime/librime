@echo off
setlocal
if exist env.bat call env.bat

if defined BOOST_ROOT (
  if exist "%BOOST_ROOT%\boost" goto boost_found
)
echo Boost not found! please set BOOST_ROOT in env.bat.
exit /b 1
:boost_found

if defined CMAKE_INSTALL_PATH (
  if exist "%CMAKE_INSTALL_PATH%\bin" goto cmake_found
)
echo Detecting CMake install path.
set CMAKE_INSTALL_PATH=%ProgramFiles(x86)%\CMake
if exist "%CMAKE_INSTALL_PATH%\bin" goto cmake_found
set CMAKE_INSTALL_PATH=%ProgramFiles%\CMake
if exist "%CMAKE_INSTALL_PATH%\bin" goto cmake_found
set CMAKE_INSTALL_PATH=
echo CMake not found! please set CMAKE_INSTALL_PATH in env.bat.
exit /b 1
:cmake_found

if defined GIT_INSTALL_PATH (
  if exist "%GIT_INSTALL_PATH%\bin" goto git_found
)
echo Detecting Git install path.
set GIT_INSTALL_PATH=%ProgramFiles(x86)%\Git
if exist "%GIT_INSTALL_PATH%\bin" goto git_found
set GIT_INSTALL_PATH=%ProgramFiles%\Git
if exist "%GIT_INSTALL_PATH%\bin" goto git_found
set GIT_INSTALL_PATH=
echo Git not found! please set GIT_INSTALL_PATH in env.bat.
exit /b 1
:git_found

if defined VS_INSTALL_PATH (
  if exist "%VS_INSTALL_PATH%\VC" goto vs_found
)
echo Detecting Visual Studio install path.
set VS_INSTALL_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio 12.0
if exist "%VS_INSTALL_PATH%\VC" goto vs_found
set VS_INSTALL_PATH=%ProgramFiles%\Microsoft Visual Studio 12.0
if exist "%VS_INSTALL_PATH%\VC" goto vs_found
set VS_INSTALL_PATH=
echo Visual Studio not found! please set VS_INSTALL_PATH in env.bat.
exit /b 1
:vs_found

:start
set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%OLD_PATH%;%DEV_PATH%

rem add cmake executable to PATH
set PATH=%PATH%;%CMAKE_INSTALL_PATH%\bin

rem git start
@rem Get the abolute path to the current directory, which is assumed to be the
@rem Git installation root.
@for /F "delims=" %%I in ("%GIT_INSTALL_PATH%") do @set git_install_root=%%~fI
@set PATH=%git_install_root%\bin;%git_install_root%\mingw\bin;%git_install_root%\cmd;%PATH%
@if not exist "%HOME%" @set HOME=%HOMEDRIVE%%HOMEPATH%
@if not exist "%HOME%" @set HOME=%USERPROFILE%
@set PLINK_PROTOCOL=ssh
@cd %HOME%
rem git end

rem start command prompt with Visual C++ tools
rem %comspec%
%comspec% /k ""%VS_INSTALL_PATH%\VC\vcvarsall.bat"" x86
