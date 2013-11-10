@echo off
rem rime build script for msvc toolchain.
rem
rem 2012-11-25 <chen.sst@gmail.com>

set BACK=%CD%

if exist env.bat call env.bat

set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%OLD_PATH%;%DEV_PATH%
path
echo.

if not defined RIME_ROOT set RIME_ROOT=%CD%
echo RIME_ROOT=%RIME_ROOT%
echo.

echo BOOST_ROOT=%BOOST_ROOT%
echo.

set build=vcbuild
set build_boost=0
set build_thirdparty=0
set build_librime=0
set build_shared=ON
set build_test=OFF

if "%1" == "" set build_librime=1

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "boost" set build_boost=1
if "%1" == "thirdparty" set build_thirdparty=1
if "%1" == "librime" set build_librime=1
if "%1" == "static" (
  set build=vcbuild-static
  set build_librime=1
  set build_shared=OFF
)
if "%1" == "shared" (
  set build=vcbuild
  set build_librime=1
  set build_shared=ON
)
if "%1" == "test" (
  set build_librime=1
  set build_test=ON
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

set CURL=%RIME_ROOT%\thirdparty\bin\curl.exe
set DOWNLOAD="%CURL%" --remote-name-all
set GOOGLECODE_SVN=http://rimeime.googlecode.com/svn/trunk/

if %build_boost% == 1 (
  cd %BOOST_ROOT%
  if not exist bjam.exe call bootstrap.bat
  if %ERRORLEVEL% NEQ 0 goto ERROR
  bjam toolset=msvc-10.0 variant=release link=static threading=multi runtime-link=static stage --with-chrono --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread
  if %ERRORLEVEL% NEQ 0 goto ERROR
  bjam toolset=msvc-10.0 variant=release link=static threading=multi runtime-link=static address-model=64 --stagedir=stage_x64 stage --with-chrono --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread
  if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_thirdparty% == 1 (
  echo building glog.
  cd "%RIME_ROOT%"\thirdparty\src\glog
  devenv google-glog-vc10.sln /build "Release"
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y src\windows\glog "%RIME_ROOT%"\thirdparty\include\glog\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libglog.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libglog.dll "%RIME_ROOT%"\thirdparty\bin\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building kyotocabinet.
  cd "%RIME_ROOT%"\thirdparty\src\kyotocabinet
  nmake -f VCmakefile
  if %ERRORLEVEL% NEQ 0 goto ERROR
  nmake -f VCmakefile binpkg
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  copy /Y output\include\*.h "%RIME_ROOT%"\thirdparty\include\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y output\lib\*.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y output\bin\*.* "%RIME_ROOT%"\thirdparty\bin\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building yaml-cpp.
  cd "%RIME_ROOT%"\thirdparty\src\yaml-cpp
  if not exist build mkdir build
  cd build
  cmake -DMSVC_SHARED_RT=OFF ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  devenv YAML_CPP.sln /build "Release"
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\include\yaml-cpp "%RIME_ROOT%"\thirdparty\include\yaml-cpp\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libyaml-cppmt.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building gtest.
  cd "%RIME_ROOT%"\thirdparty\src\gtest
  if not exist build mkdir build
  cd build
  cmake ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  devenv gtest.sln /build "Release"
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\include\gtest "%RIME_ROOT%"\thirdparty\include\gtest\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\gtest*.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo skipped building opencc.

  cd "%RIME_ROOT%"\thirdparty\include
  if not exist opencc mkdir opencc
  cd opencc
  if not exist opencc.h %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/opencc.h
  if not exist opencc_types.h %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/opencc_types.h
  cd "%RIME_ROOT%"\thirdparty\lib
  if not exist opencc.lib %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/vc10/opencc.lib
  cd "%RIME_ROOT%"\thirdparty\bin
  if not exist opencc.dll %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/opencc.dll
  if not exist opencc.exe %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/opencc.exe
  if not exist opencc_dict.exe %DOWNLOAD% %GOOGLECODE_SVN%misc/opencc/opencc_dict.exe
  if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_librime% == 0 goto EXIT

set CMAKE_INCLUDE_PATH=%RIME_ROOT%\thirdparty\include
echo CMAKE_INCLUDE_PATH=%CMAKE_INCLUDE_PATH%
echo.
set CMAKE_LIBRARY_PATH=%RIME_ROOT%\thirdparty\lib
echo CMAKE_LIBRARY_PATH=%CMAKE_LIBRARY_PATH%
echo.

rem TODO: select a cmake generator
rem set CMAKE_GENERATOR="MinGW Makefiles"
rem set CMAKE_GENERATOR="Eclipse CDT4 - MinGW Makefiles"
rem set CMAKE_GENERATOR="Visual Studio 9 2008"
set CMAKE_GENERATOR="Visual Studio 10"

set BUILD_DIR=%RIME_ROOT%\%build%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

set RIME_CMAKE_FLAGS=-DBUILD_STATIC=ON -DBUILD_SHARED_LIBS=%build_shared% -DBUILD_TEST=%build_test%

cd %BUILD_DIR%
echo cmake -G %CMAKE_GENERATOR% %RIME_CMAKE_FLAGS% %RIME_ROOT%
cmake -G %CMAKE_GENERATOR% %RIME_CMAKE_FLAGS% %RIME_ROOT%
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo building librime.
if exist %build%.log del %build%.log
devenv rime.sln /Build Release /Out %build%.log
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo ready.
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
