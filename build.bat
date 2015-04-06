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

rem set CURL=%RIME_ROOT%\thirdparty\bin\curl.exe
rem set DOWNLOAD="%CURL%" --remote-name-all

set LEVELDB_REPOSITORY=lotem/leveldb

if %build_boost% == 1 (
  cd %BOOST_ROOT%
  if not exist bjam.exe call bootstrap.bat
  if %ERRORLEVEL% NEQ 0 goto ERROR
  bjam toolset=msvc-12.0 variant=release link=static threading=multi runtime-link=static stage --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread
  if %ERRORLEVEL% NEQ 0 goto ERROR
  rem bjam toolset=msvc-12.0 variant=release link=static threading=multi runtime-link=static address-model=64 --stagedir=stage_x64 stage --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread
  rem if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_thirdparty% == 1 (
  echo building glog.
  cd "%RIME_ROOT%"\thirdparty\src\glog
  msbuild.exe google-glog-vc12.sln /p:Configuration=Release
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
  nmake -f VC12makefile
  if %ERRORLEVEL% NEQ 0 goto ERROR
  nmake -f VC12makefile binpkg
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  copy /Y output\include\*.h "%RIME_ROOT%"\thirdparty\include\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y output\lib\*.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y output\bin\*.* "%RIME_ROOT%"\thirdparty\bin\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building leveldb.
  cd "%RIME_ROOT%"\thirdparty\src\
  echo "checking out 'windows' branch from %LEVELDB_REPOSITORY%"
  git clone -b windows git@github.com:%LEVELDB_REPOSITORY%.git leveldb-windows
  if %ERRORLEVEL% NEQ 0 goto ERROR
  cd leveldb-windows
  echo BOOST_ROOT=%BOOST_ROOT%
  msbuild.exe leveldb.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y include\leveldb "%RIME_ROOT%"\thirdparty\include\leveldb\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\leveldb.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building yaml-cpp.
  cd "%RIME_ROOT%"\thirdparty\src\yaml-cpp
  if not exist build mkdir build
  cd build
  cmake -DMSVC_SHARED_RT=OFF ..
  if %ERRORLEVEL% NEQ 0 goto ERROR
  msbuild.exe YAML_CPP.sln /p:Configuration=Release
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
  msbuild.exe gtest.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\include\gtest "%RIME_ROOT%"\thirdparty\include\gtest\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\gtest*.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building marisa.
  cd "%RIME_ROOT%"\thirdparty\src\marisa-trie\vs2013
  msbuild.exe vs2013.sln /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  xcopy /S /I /Y ..\lib\marisa "%RIME_ROOT%"\thirdparty\include\marisa\
  xcopy /Y ..\lib\marisa.h "%RIME_ROOT%"\thirdparty\include\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\libmarisa.lib "%RIME_ROOT%"\thirdparty\lib\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  copy /Y Release\marisa-*.exe "%RIME_ROOT%"\thirdparty\bin\
  if %ERRORLEVEL% NEQ 0 goto ERROR

  echo building opencc.
  cd "%RIME_ROOT%"\thirdparty\src\opencc
  if not exist build mkdir build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=""
  echo patching src\libopencc.vcxproj for static linking runtime.
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\libopencc.vcxproj
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\opencc.vcxproj
  sed -i "s/MultiThreadedDLL/MultiThreaded/" src\opencc_dict.vcxproj
  msbuild.exe opencc.sln /t:libopencc;opencc;opencc_dict /p:Configuration=Release
  if %ERRORLEVEL% NEQ 0 goto ERROR
  echo built. copying artifacts.
  cd "%RIME_ROOT%"\thirdparty\include
  if not exist opencc mkdir opencc
  copy /Y "%RIME_ROOT%"\thirdparty\src\opencc\src\*.h* opencc\
  if %ERRORLEVEL% NEQ 0 goto ERROR
  cd "%RIME_ROOT%"\thirdparty\lib
  copy /Y "%RIME_ROOT%"\thirdparty\src\opencc\build\src\Release\opencc.lib .
  if %ERRORLEVEL% NEQ 0 goto ERROR
  cd "%RIME_ROOT%"\thirdparty\bin
  copy /Y "%RIME_ROOT%"\thirdparty\src\opencc\build\src\Release\opencc.dll .
  copy /Y "%RIME_ROOT%"\thirdparty\src\opencc\build\src\Release\opencc.exe .
  copy /Y "%RIME_ROOT%"\thirdparty\src\opencc\build\src\Release\opencc_dict.exe .
  if %ERRORLEVEL% NEQ 0 goto ERROR
)

if %build_librime% == 0 goto EXIT

set CMAKE_GENERATOR="Visual Studio 12"

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
echo.
echo error building la rime.
echo.

:EXIT
set PATH=%OLD_PATH%
cd %RIME_ROOT%
rem pause
