rem Customize your build environment and save the modified copy to env.bat

set RIME_ROOT=%CD%

rem REQUIRED: path to Boost source directory
if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\thirdparty\src\boost_1_69_0

rem OPTIONAL: architecture, Visual Studio version and platform toolset
set ARCH=Win32
set BJAM_TOOLSET=msvc-14.1
set CMAKE_GENERATOR="Visual Studio 15 2017"
set PLATFORM_TOOLSET=v141_xp

rem OPTIONAL: path to additional build tools
rem set DEVTOOLS_PATH=%ProgramFiles%\Git\cmd;%ProgramFiles%\CMake\bin;C:\Python27;
