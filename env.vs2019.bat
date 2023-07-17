rem Customize your build environment and save the modified copy to env.bat

set RIME_ROOT=%CD%

rem REQUIRED: path to Boost source directory
if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost_1_78_0

rem architecture, Visual Studio version and compiler
set "VSVEROPT=-version [16.0^,17.0^)"
call ./env.vswhere.bat x86
set VSVEROPT=
set CXX=cl
set CC=cl

rem OPTIONAL: path to additional build tools
rem set DEVTOOLS_PATH=%ProgramFiles%\Git\cmd;%ProgramFiles%\CMake\bin;C:\Python27;
