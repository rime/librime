@echo off
rem Rime build script for msvc toolchain.
rem Maintainer: Chen Gong <chen.sst@gmail.com>

setlocal
set BACK=%CD%

if not exist env.bat copy env.bat.template env.bat

if exist env.bat call env.bat

rem for Windows XP compatibility (Visual Studio 2015+)
set CL=/Zc:threadSafeInit-

set OLD_PATH=%PATH%
if defined DEVTOOLS_PATH set PATH=%OLD_PATH%;%DEVTOOLS_PATH%
path
echo.

if not defined RIME_ROOT set RIME_ROOT=%CD%
echo RIME_ROOT=%RIME_ROOT%
echo.

if defined BOOST_ROOT (
  if exist "%BOOST_ROOT%\boost" goto boost_found
)
echo Error: Boost not found! Please set BOOST_ROOT in env.bat.
exit /b 1
:boost_found
echo BOOST_ROOT=%BOOST_ROOT%
echo.

if not defined ARCH (
  set ARCH=Win32
)

if not defined BJAM_TOOLSET (
  rem the number actually means platform toolset, not %VisualStudioVersion%
  set BJAM_TOOLSET=msvc-14.2
)

if not defined CMAKE_GENERATOR (
  set CMAKE_GENERATOR="Visual Studio 16 2019"
)

if not defined PLATFORM_TOOLSET (
  set PLATFORM_TOOLSET=v142
)

set clean=0
set build_dir_base=build
set build_dir_suffix=
set build_config=Release
set build_boost=0
set build_boost_x64=0
set boost_build_variant=release
set build_thirdparty=0
set build_librime=0
set build_shared=ON
set build_test=OFF
set enable_logging=ON

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "clean" set clean=1
if "%1" == "boost" set build_boost=1
if "%1" == "boost_x64" (
  set build_boost=1
  set build_boost_x64=1
)
if "%1" == "thirdparty" set build_thirdparty=1
if "%1" == "librime" set build_librime=1
if "%1" == "static" (
  set build_dir_suffix=-static
  set build_shared=OFF
)
if "%1" == "shared" (
  set build_dir_suffix=
  set build_shared=ON
)
if "%1" == "test" (
  set build_librime=1
  set build_test=ON
)
if "%1" == "debug" (
  set build_dir_base=debug
  set build_config=Debug
  set boost_build_variant=debug
)
if "%1" == "release" (
  set build_dir_base=build
  set build_config=Release
  set boost_build_variant=release

)
if "%1" == "logging" (
  set enable_logging=ON
)
if "%1" == "nologging" (
  set enable_logging=OFF
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

if %clean% == 0 (
if %build_librime% == 0 (
if %build_boost% == 0 (
if %build_thirdparty% == 0 (
  set build_librime=1
))))

if %clean% == 1 (
  rmdir /s /q build
  rmdir /s /q thirdparty\src\capnproto\build
  rmdir /s /q thirdparty\src\glog\cmake-build
  rmdir /s /q thirdparty\src\googletest\build
  rmdir /s /q thirdparty\src\leveldb\build
  rmdir /s /q thirdparty\src\marisa-trie\build
  rmdir /s /q thirdparty\src\opencc\build
  rmdir /s /q thirdparty\src\yaml-cpp\build
)

set build_dir=%build_dir_base%%build_dir_suffix%

set DIST_DIR=%RIME_ROOT%\dist
set THIRDPARTY=%RIME_ROOT%\thirdparty

rem set CURL=%THIRDPARTY%\bin\curl.exe
rem set DOWNLOAD="%CURL%" --remote-name-all

set BOOST_COMPILED_LIBS=--with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-system^
 --with-thread

set BJAM_OPTIONS_COMMON=toolset=%BJAM_TOOLSET%^
 variant=%boost_build_variant%^
 link=static^
 threading=multi^
 runtime-link=static^
 cxxflags="/Zc:threadSafeInit- "

set BJAM_OPTIONS_X86=%BJAM_OPTIONS_COMMON%^
 define=BOOST_USE_WINAPI_VERSION=0x0501

set BJAM_OPTIONS_X64=%BJAM_OPTIONS_COMMON%^
 define=BOOST_USE_WINAPI_VERSION=0x0502^
 address-model=64^
 --stagedir=stage_x64

if %build_boost% == 1 (
  cd /d %BOOST_ROOT%
  if not exist b2.exe call bootstrap.bat
  if errorlevel 1 goto error

  b2 %BJAM_OPTIONS_X86% stage %BOOST_COMPILED_LIBS%
  if errorlevel 1 goto error

  if %build_boost_x64% == 1 (
    b2 %BJAM_OPTIONS_X64% stage %BOOST_COMPILED_LIBS%
    if errorlevel 1 goto error
  )
)

set THIRDPARTY_COMMON_CMAKE_FLAGS=-G%CMAKE_GENERATOR%^
 -A%ARCH%^
 -T%PLATFORM_TOOLSET%^
 -DCMAKE_CONFIGURATION_TYPES:STRING="%build_config%"^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_C_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_INSTALL_PREFIX:PATH="%THIRDPARTY%"

if %build_thirdparty% == 1 (
  cd /d %THIRDPARTY%

  echo building capnproto.
  cd %THIRDPARTY%\src\capnproto
  cmake . -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
	-DBUILD_SHARED_LIBS:BOOL=OFF^
	-DBUILD_TESTING:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building glog.
  cd %THIRDPARTY%\src\glog
  cmake . -Bcmake-%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
  -DBUILD_SHARED_LIBS:BOOL=OFF^
  -DBUILD_TESTING:BOOL=OFF^
  -DWITH_GFLAGS:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build cmake-%build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building leveldb.
  cd %THIRDPARTY%\src\leveldb
  cmake . -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
  -DLEVELDB_BUILD_BENCHMARKS:BOOL=OFF^
  -DLEVELDB_BUILD_TESTS:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building yaml-cpp.
  cd %THIRDPARTY%\src\yaml-cpp
  cmake . -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
  -DMSVC_SHARED_RT:BOOL=OFF^
  -DYAML_MSVC_SHARED_RT:BOOL=OFF^
  -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF^
  -DYAML_CPP_BUILD_TESTS:BOOL=OFF^
  -DYAML_CPP_BUILD_TOOLS:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building gtest.
  cd %THIRDPARTY%\src\googletest
  cmake . -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
  -DBUILD_GMOCK:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building marisa.
  cd %THIRDPARTY%\src\marisa-trie
  cmake %THIRDPARTY%\src -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error

  echo building opencc.
  cd %THIRDPARTY%\src\opencc
  cmake . -B%build_dir% %THIRDPARTY_COMMON_CMAKE_FLAGS%^
  -DBUILD_SHARED_LIBS=OFF^
  -DBUILD_TESTING=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target INSTALL
  if errorlevel 1 goto error
)

if %build_librime% == 0 goto exit

set RIME_CMAKE_FLAGS=-G%CMAKE_GENERATOR%^
 -A%ARCH%^
 -T%PLATFORM_TOOLSET%^
 -DBUILD_STATIC=ON^
 -DBUILD_SHARED_LIBS=%build_shared%^
 -DBUILD_TEST=%build_test%^
 -DENABLE_LOGGING=%enable_logging%^
 -DBOOST_USE_CXX11=ON^
 -DCMAKE_CONFIGURATION_TYPES="%build_config%"^
 -DCMAKE_INSTALL_PREFIX:PATH="%DIST_DIR%"

cd /d %RIME_ROOT%
echo cmake %RIME_ROOT% -B%build_dir% %RIME_CMAKE_FLAGS%
call cmake %RIME_ROOT% -B%build_dir% %RIME_CMAKE_FLAGS%
if errorlevel 1 goto error

echo.
echo building librime.
cmake --build %build_dir% --config %build_config% --target INSTALL
if errorlevel 1 goto error

echo.
echo ready.
echo.
goto exit

:error
set exitcode=%errorlevel%
echo.
echo error building la rime.
echo.

:exit
set PATH=%OLD_PATH%
cd /d %BACK%
exit /b %exitcode%
