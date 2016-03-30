@echo off
rem Rime build script for msvc toolchain.
rem 2014-12-30  Chen Gong <chen.sst@gmail.com>

setlocal
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

if defined CMAKE_INSTALL_PATH set PATH=%PATH%;%CMAKE_INSTALL_PATH%

set CMAKE_GENERATOR="Visual Studio 14 2015"
set CMAKE_TOOLSET="v140_xp"

set build=build
set build_boost=0
set build_thirdparty=0
set build_librime=0
set build_shared=ON
set build_test=OFF
set enable_logging=ON

if "%1" == "" set build_librime=1

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "boost" set build_boost=1
if "%1" == "thirdparty" set build_thirdparty=1
if "%1" == "librime" set build_librime=1
if "%1" == "static" (
  set build=build-static
  set build_librime=1
  set build_shared=OFF
)
if "%1" == "shared" (
  set build=build
  set build_librime=1
  set build_shared=ON
)
if "%1" == "test" (
  set build_librime=1
  set build_test=ON
)
if "%1" == "nologging" (
  set build_librime=1
  set enable_logging=OFF
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

set THIRDPARTY="%RIME_ROOT%"\thirdparty

rem set CURL=%THIRDPARTY%\bin\curl.exe
rem set DOWNLOAD="%CURL%" --remote-name-all

if %build_boost% == 1 (
  cd %BOOST_ROOT%
  if not exist bjam.exe call bootstrap.bat
  if %ERRORLEVEL% NEQ 0 goto ERROR
  bjam toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static stage --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread --with-locale
  if %ERRORLEVEL% NEQ 0 goto ERROR
  rem bjam toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static address-model=64 --stagedir=stage_x64 stage --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread
  rem if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_thirdparty% == 1 (
  echo building glog.
  cd %THIRDPARTY%\src\glog
  if not exist build mkdir build
  cd build
  cmake -G %CMAKE_GENERATOR% -T %CMAKE_TOOLSET% -DWITH_GFLAGS=OFF -DCMAKE_CONFIGURATION_TYPES="Release" -DCMAKE_CXX_FLAGS_RELEASE="/MT /O2 /Ob2 /D NDEBUG" -DCMAKE_C_FLAGS_RELEASE="/MT /O2 /Ob2 /D NDEBUG" ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  msbuild.exe google-glog.sln /t:glog /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\src\windows\glog %THIRDPARTY%\include\glog\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\glog.lib %THIRDPARTY%\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building leveldb.
  cd %THIRDPARTY%\src\leveldb-windows
  echo BOOST_ROOT=%BOOST_ROOT%
  msbuild.exe leveldb.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y include\leveldb %THIRDPARTY%\include\leveldb\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\leveldb.lib %THIRDPARTY%\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building yaml-cpp.
  cd %THIRDPARTY%\src\yaml-cpp
  if not exist build mkdir build
  cd build
  cmake -G %CMAKE_GENERATOR% -T %CMAKE_TOOLSET% -DMSVC_SHARED_RT=OFF -DCMAKE_CONFIGURATION_TYPES="Release" -DYAML_CPP_BUILD_TOOLS=OFF ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  msbuild.exe YAML_CPP.sln /t:yaml-cpp /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\include\yaml-cpp %THIRDPARTY%\include\yaml-cpp\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libyaml-cppmt.lib %THIRDPARTY%\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building gtest.
  cd %THIRDPARTY%\src\gtest
  if not exist build mkdir build
  cd build
  cmake ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  msbuild.exe gtest.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\include\gtest %THIRDPARTY%\include\gtest\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\gtest*.lib %THIRDPARTY%\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building marisa.
  cd %THIRDPARTY%\src\marisa-trie\vs2013
  msbuild.exe vs2013.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\lib\marisa %THIRDPARTY%\include\marisa\
  xcopy /Y ..\lib\marisa.h %THIRDPARTY%\include\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libmarisa.lib %THIRDPARTY%\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\marisa-*.exe %THIRDPARTY%\bin\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building opencc.
  cd %THIRDPARTY%\src\opencc
  if not exist build mkdir build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX="" -DBUILD_SHARED_LIBS=OFF
  echo patching src\libopencc.vcxproj for static linking runtime.
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\libopencc.vcxproj
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\tools\opencc.vcxproj
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\tools\opencc_dict.vcxproj
  msbuild.exe opencc.sln /t:libopencc;opencc;opencc_dict;Dictionaries /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  cd ..
  if not exist %THIRDPARTY%\include\opencc mkdir %THIRDPARTY%\include\opencc
  copy /Y src\*.h* %THIRDPARTY%\include\opencc\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y build\src\Release\opencc.lib %THIRDPARTY%\lib
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y build\src\tools\Release\opencc.exe %THIRDPARTY%\bin
  copy /Y build\src\tools\Release\opencc_dict.exe %THIRDPARTY%\bin
  if %ERRORLEVEL% NEQ 0 goto ERROR
  if not exist %THIRDPARTY%\data\opencc mkdir %THIRDPARTY%\data\opencc
  copy /Y data\config\*.json %THIRDPARTY%\data\opencc
  copy /Y build\data\*.ocd %THIRDPARTY%\data\opencc
  if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_librime% == 0 goto EXIT

set BUILD_DIR=%RIME_ROOT%\%build%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

set RIME_CMAKE_FLAGS=-DBUILD_STATIC=ON -DBUILD_SHARED_LIBS=%build_shared% -DBUILD_TEST=%build_test% -DENABLE_LOGGING=%enable_logging% -DBOOST_USE_CXX11=ON

cd %BUILD_DIR%
echo cmake -G %CMAKE_GENERATOR% %RIME_CMAKE_FLAGS% %RIME_ROOT%
call cmake -G %CMAKE_GENERATOR% %RIME_CMAKE_FLAGS% %RIME_ROOT%
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo building librime.
if exist msbuild.log del msbuild.log
msbuild.exe rime.sln /p:Configuration=Release /fl
if %ERRORLEVEL% NEQ 0 goto ERROR

echo.
echo ready.
echo.
goto EXIT

:ERROR
set EXITCODE=%ERRORLEVEL%
echo.
echo error building la rime.
echo.

:EXIT
set PATH=%OLD_PATH%
cd %BACK%
rem pause
exit /b %EXITCODE%
