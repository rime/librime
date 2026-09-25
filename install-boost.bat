setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%

for /f "usebackq eol=# tokens=1,* delims==" %%A in ("%RIME_ROOT%\action-versions.sh") do (
  if not defined %%A set "%%A=%%B"
)

if not defined boost_tarball set boost_tarball=boost_%boost_version:.=_%

if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost

if exist "%BOOST_ROOT%\libs" goto boost_found
for %%I in ("%BOOST_ROOT%\.") do set src_dir=%%~dpI
rem download boost source
aria2c https://archives.boost.io/release/%boost_version%/source/%boost_tarball%.7z -d %src_dir%
pushd %src_dir%
7z x %boost_tarball%.7z
ren %boost_tarball% boost
cd boost
call .\bootstrap.bat
.\b2 headers
popd
:boost_found
