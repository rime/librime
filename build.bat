@echo off
rem Rime build script for msvc toolchain.
rem Maintainer: Chen Gong <chen.sst@gmail.com>

setlocal

if not exist env.bat copy env.bat.template env.bat

if exist env.bat call .\env.bat

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

set clean=0
set build_dir_base=build
set build_dir_suffix=
set build_config=Release
set build_deps=0
set build_librime=0
set build_shared=ON
set build_test=OFF
set enable_logging=ON

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "clean" set clean=1
if "%1" == "deps" set build_deps=1
rem `thirdparty` is deprecated in favor of `deps`
if "%1" == "thirdparty" set build_deps=1
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
)
if "%1" == "release" (
  set build_dir_base=build
  set build_config=Release
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
if %build_deps% == 0 (
  set build_librime=1
)))

if %clean% == 1 (
  rmdir /s /q build
  rmdir /s /q deps\glog\build
  rmdir /s /q deps\googletest\build
  rmdir /s /q deps\leveldb\build
  rmdir /s /q deps\marisa-trie\build
  rmdir /s /q deps\opencc\build
  rmdir /s /q deps\yaml-cpp\build
)

set build_dir=%build_dir_base%%build_dir_suffix%

if defined CMAKE_GENERATOR (
  set common_cmake_flags=%common_cmake_flags% -G%CMAKE_GENERATOR%
)
if defined ARCH (
  set common_cmake_flags=%common_cmake_flags% -A%ARCH%
)
if defined PLATFORM_TOOLSET (
  set common_cmake_flags=%common_cmake_flags% -T%PLATFORM_TOOLSET%
)
set deps_cmake_flags=%common_cmake_flags%^
 -DCMAKE_CONFIGURATION_TYPES:STRING="%build_config%"^
 -DCMAKE_BUILD_TYPE:STRING="%build_config%"^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /O2 /Ob2 /DNDEBUG"^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_C_FLAGS_DEBUG:STRING="/MTd /Od"^
 -DCMAKE_INSTALL_PREFIX:PATH="%RIME_ROOT%"

if %build_deps% == 1 (
  echo building glog.
  pushd deps\glog
  cmake . -B%build_dir% %deps_cmake_flags%^
  -DBUILD_SHARED_LIBS:BOOL=OFF^
  -DBUILD_TESTING:BOOL=OFF^
  -DWITH_GFLAGS:BOOL=OFF^
  -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd

  echo building leveldb.
  pushd deps\leveldb
  cmake . -B%build_dir% %deps_cmake_flags%^
  -DLEVELDB_BUILD_BENCHMARKS:BOOL=OFF^
  -DLEVELDB_BUILD_TESTS:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd

  echo building yaml-cpp.
  pushd deps\yaml-cpp
  cmake . -B%build_dir% %deps_cmake_flags%^
  -DMSVC_SHARED_RT:BOOL=OFF^
  -DYAML_MSVC_SHARED_RT:BOOL=OFF^
  -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF^
  -DYAML_CPP_BUILD_TESTS:BOOL=OFF^
  -DYAML_CPP_BUILD_TOOLS:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd

  echo building gtest.
  pushd deps\googletest
  cmake . -B%build_dir% %deps_cmake_flags%^
  -DBUILD_GMOCK:BOOL=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd

  echo building marisa.
  pushd deps\marisa-trie
  cmake . -B%build_dir% %deps_cmake_flags%
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd

  echo building opencc.
  pushd deps\opencc
  cmake . -B%build_dir% %deps_cmake_flags%^
  -DBUILD_SHARED_LIBS=OFF^
  -DBUILD_TESTING=OFF
  if errorlevel 1 goto error
  cmake --build %build_dir% --config %build_config% --target install
  if errorlevel 1 goto error
  popd
)

if %build_librime% == 0 goto exit

set rime_cmake_flags=%common_cmake_flags%^
 -DBUILD_STATIC=ON^
 -DBUILD_SHARED_LIBS=%build_shared%^
 -DBUILD_TEST=%build_test%^
 -DENABLE_LOGGING=%enable_logging%^
 -DCMAKE_CONFIGURATION_TYPES="%build_config%"^
 -DCMAKE_BUILD_TYPE:STRING="%build_config%"^
 -DCMAKE_INSTALL_PREFIX:PATH="%RIME_ROOT%\dist"

echo on
call cmake . -B%build_dir% %rime_cmake_flags%
@echo off
if errorlevel 1 goto error

echo.
echo building librime.
echo.
echo on
cmake --build %build_dir% --config %build_config% --target install
@echo off
if errorlevel 1 goto error

if "%build_test%" == "ON" (
  pushd %build_dir%
  ctest --output-on-failure
  popd
)

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
exit /b %exitcode%
