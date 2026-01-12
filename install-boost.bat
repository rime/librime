setlocal EnableDelayedExpansion

if not defined RIME_ROOT set RIME_ROOT=%CD%

if not defined boost_version set boost_version=1.89.0

if not defined boost_tarball set boost_tarball=boost_%boost_version:.=_%

if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost-%boost_version%
if not defined BOOST_PREFIX set BOOST_PREFIX=%RIME_ROOT%
if not defined boost_libs set boost_libs=regex,locale
if not defined BOOST_TOOLSET (
  echo ERROR: BOOST_TOOLSET is not set. Set BOOST_TOOLSET to msvc or clang-win.
  exit /b 1
)
set BOOST_CONFIG_DIR=%BOOST_PREFIX%\lib\cmake\Boost-%boost_version%
set BOOST_CONFIG_FILE=%BOOST_CONFIG_DIR%\BoostConfig.cmake
set boost_needs_build=1
set BOOST_WITH_LIBS=
for %%L in (%boost_libs:,= %) do (
  set BOOST_WITH_LIBS=!BOOST_WITH_LIBS! --with-%%L
)

set do_build=0
for %%A in (%*) do (
  if /I "%%A"=="--build" set do_build=1
)

if exist "%BOOST_ROOT%\libs" goto boost_found
for %%I in ("%BOOST_ROOT%\.") do set src_dir=%%~dpI
rem download boost source
aria2c https://archives.boost.io/release/%boost_version%/source/%boost_tarball%.7z -d %src_dir%
pushd %src_dir%
7z x %boost_tarball%.7z
ren %boost_tarball% boost-%boost_version%
cd boost-%boost_version%
call .\bootstrap.bat --with-libraries=%boost_libs% --with-toolset=%BOOST_TOOLSET%
.\b2 headers
popd
:boost_found

if %do_build%==1 (
  if exist "%BOOST_CONFIG_FILE%" set boost_needs_build=0
  if %boost_needs_build%==1 (
    pushd %BOOST_ROOT%
    if not exist b2.exe (
      call .\bootstrap.bat --with-libraries=%boost_libs% --with-toolset=%BOOST_TOOLSET%
    )
    .\b2 -q -a link=static toolset=%BOOST_TOOLSET% %BOOST_WITH_LIBS% install --prefix="%BOOST_PREFIX%"
    popd
  ) else (
    echo Boost already installed at "%BOOST_PREFIX%". Skipping build.
  )
)
