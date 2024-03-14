setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%

if not defined boost_version set boost_version=1.84.0

if not defined BOOST_ROOT set BOOST_ROOT=%RIME_ROOT%\deps\boost-%boost_version%

if exist "%BOOST_ROOT%\libs" goto boost_found
for %%I in ("%BOOST_ROOT%\.") do set src_dir=%%~dpI
rem download boost source
aria2c https://github.com/boostorg/boost/releases/download/boost-%boost_version%/boost-%boost_version%.7z -d %src_dir%
pushd %src_dir%
7z x boost-%boost_version%.7z
cd boost-%boost_version%
call .\bootstrap.bat
.\b2 headers
popd
:boost_found
