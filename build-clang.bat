@echo off

if "%BOOST_ROOT%" == "" (
  echo Please set BOOST_ROOT environment variable.
  exit
)

set RIME_ROOT=%CD%

@REM Build for native architecture
set arch=x86
if "%PROCESSOR_ARCHITECTURE%" == "ARM64" set arch=arm

set clean=0
set build_deps=0
set build_librime=0
set build_test=OFF

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if "%1" == "clean" set clean=1
if "%1" == "deps" set build_deps=1
if "%1" == "librime" set build_librime=1
if "%1" == "test" (
  set build_librime=1
  set build_test=ON
)
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

if %clean% == 0 (
if %build_deps% == 0 (
  set build_librime=1
))

if %clean% == 1 (
  rmdir /s /q build
  rmdir /s /q deps\glog\build
  rmdir /s /q deps\googletest\build
  rmdir /s /q deps\leveldb\build
  rmdir /s /q deps\marisa-trie\build
  rmdir /s /q deps\opencc\build
  rmdir /s /q deps\yaml-cpp\build
)

set common_cmake_flags=-B build^
  -G Ninja^
  -DCMAKE_C_COMPILER=clang^
  -DCMAKE_CXX_COMPILER=clang++^
  -DCMAKE_BUILD_TYPE:STRING=Release^
  -DCMAKE_USER_MAKE_RULES_OVERRIDE:PATH="%RIME_ROOT%\cmake\c_flag_overrides.cmake"^
  -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX:PATH="%RIME_ROOT%\cmake\cxx_flag_overrides.cmake"^
  -DCMAKE_EXE_LINKER_FLAGS_INIT:STRING="-llibcmt"^
  -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded

set deps_cmake_flags=%common_cmake_flags%^
  -DBUILD_SHARED_LIBS:BOOL=OFF^
  -DCMAKE_INSTALL_PREFIX:PATH="%RIME_ROOT%"

if %build_deps% == 1 (
  echo building glog.
  pushd deps\glog
  cmake . %deps_cmake_flags%^
    -DWITH_GFLAGS:BOOL=OFF^
    -DBUILD_TESTING:BOOL=OFF || exit
  cmake --build build || exit
  cmake --install build || exit
  popd

  echo building leveldb.
  pushd deps\leveldb
  cmake . %deps_cmake_flags%^
    -DCMAKE_CXX_FLAGS:STRING="-Wno-error=deprecated-declarations"^
    -DLEVELDB_BUILD_BENCHMARKS:BOOL=OFF^
    -DLEVELDB_BUILD_TESTS:BOOL=OFF || exit
  cmake --build build || exit
  cmake --install build || exit
  popd

  echo building yaml-cpp.
  pushd deps\yaml-cpp
  cmake . %deps_cmake_flags%^
    -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF^
    -DYAML_CPP_BUILD_TESTS:BOOL=OFF^
    -DYAML_CPP_BUILD_TOOLS:BOOL=OFF || exit
  cmake --build build || exit
  cmake --install build || exit
  popd

  echo building gtest.
  pushd deps\googletest
  cmake . %deps_cmake_flags%^
    -DBUILD_GMOCK:BOOL=OFF || exit
  cmake --build build || exit
  cmake --install build || exit
  popd

  echo building marisa.
  pushd deps\marisa-trie
  cmake . %deps_cmake_flags% || exit
  cmake --build build || exit
  cmake --install build || exit
  popd

  echo building opencc.
  pushd deps\opencc
  cmake . %deps_cmake_flags%^
    -DCMAKE_CXX_FLAGS:STRING="-I %RIME_ROOT%\include -L %RIME_ROOT%\lib"^
    -DUSE_SYSTEM_MARISA:BOOL=ON || exit
  cmake --build build || exit
  cmake --install build || exit
  popd
)

set rime_cmake_flags=%common_cmake_flags%^
  -DBUILD_STATIC:BOOL=ON^
  -DBUILD_SHARED_LIBS:BOOL=ON^
  -DBUILD_TEST:BOOL="%build_test%"^
  -DENABLE_LOGGING:BOOL=ON^
  -DCMAKE_INSTALL_PREFIX:PATH="%RIME_ROOT%\dist"

if %build_librime% == 1 (
  echo building librime.
  cmake . %rime_cmake_flags% || exit
  cmake --build build || exit
  cmake --install build || exit
)

if "%build_test%" == "ON" (
  pushd build
  ctest --output-on-failure
  popd
)
