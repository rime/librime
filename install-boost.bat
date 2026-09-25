setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%
set BOOST_DATA_FILE=%RIME_ROOT%\boost-data.txt
if not exist "%BOOST_DATA_FILE%" (
  if exist "%~dp0boost-data.txt" for %%I in ("%~dp0.") do set RIME_ROOT=%%~fI
)
set BOOST_DATA_FILE=%RIME_ROOT%\boost-data.txt
if not exist "%BOOST_DATA_FILE%" (
  echo Error: boost-data.txt not found in %RIME_ROOT%.
  exit /b 1
)

for /f "usebackq tokens=1,* delims==" %%A in ("%BOOST_DATA_FILE%") do (
  if /i "%%A"=="version" if not defined boost_version set "boost_version=%%B"
  if /i "%%A"=="sha256sum" if not defined boost_sha256sum set "boost_sha256sum=%%B"
)
if not defined boost_version (
  echo Error: missing version in %BOOST_DATA_FILE%.
  exit /b 1
)
if not defined boost_sha256sum (
  echo Error: missing sha256sum in %BOOST_DATA_FILE%.
  exit /b 1
)

if not defined boost_tarball set boost_tarball=boost_%boost_version:.=_%

if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost-%boost_version%

if exist "%BOOST_ROOT%\libs" goto boost_found
for %%I in ("%BOOST_ROOT%\.") do set src_dir=%%~dpI
rem download boost source
aria2c https://archives.boost.io/release/%boost_version%/source/%boost_tarball%.7z -d %src_dir%
pushd %src_dir%
7z x %boost_tarball%.7z
ren %boost_tarball% boost-%boost_version%
cd boost-%boost_version%
call .\bootstrap.bat
.\b2 headers
popd
:boost_found
