setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%

if not defined boost_version set boost_version=1.89.0

if not defined boost_tarball set boost_tarball=boost_%boost_version:.=_%

if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost-%boost_version%
if not defined BOOST_PREFIX set BOOST_PREFIX=%RIME_ROOT%
if not defined boost_libs set boost_libs=regex,locale

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
call .\bootstrap.bat --with-libraries=%boost_libs%
.\b2 headers
popd
:boost_found

if %do_build%==1 (
  pushd %BOOST_ROOT%
  call .\bootstrap.bat --with-libraries=%boost_libs%
  .\b2 -q -a link=static --with-libraries=%boost_libs% install --prefix="%BOOST_PREFIX%"
  popd
)
