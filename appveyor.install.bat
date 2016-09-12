setlocal

set nocache=0

if not exist thirdparty.cached set nocache=1
if not exist %BOOST_ROOT% set nocache=1

if %nocache% == 1 (
	pushd C:\Libraries
	curl -fsSO http://superb-sea2.dl.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.7z
	7z x boost_1_61_0.7z | find "ing archive"
	cd boost_1_61_0
	call .\bootstrap.bat
	call .\b2.exe --prefix=%BOOST_ROOT% toolset=msvc-14.0 variant=release link=static threading=multi runtime-link=static --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread --with-locale -q -d0 install
	xcopy /e /i /y /q %BOOST_ROOT%\include\boost-1_61\boost %BOOST_ROOT%\boost
	popd
	if %ERRORLEVEL% NEQ 0 goto ERROR

	call .\build.bat thirdparty
	if %ERRORLEVEL% NEQ 0 goto ERROR

	date /t > thirdparty.cached & time /t >> thirdparty.cached
	echo.
	echo Thirdparty libraries installed.
	echo.
	goto EXIT
) else (
	echo.
	echo Last build date of cache is
	type thirdparty.cached
	echo.
	goto EXIT
)

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
