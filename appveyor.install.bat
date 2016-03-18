setlocal

set nocache=0

if exist thirdparty.cached (
	echo.
	echo Last build date of cache is
	type thirdparty.cached
	echo.
) else set nocache=1

if not exist %BOOST_ROOT% set nocache=1

if %nocache% == 1 (
	rmdir /s /q %BOOST_ROOT%
	pushd C:\Libraries\boost_1_59_0
	call .\bootstrap
	call .\b2 --prefix=%BOOST_ROOT% toolset=msvc-12.0 variant=release link=static threading=multi runtime-link=static --with-date_time --with-filesystem --with-system --with-regex --with-signals --with-thread --with-locale -q -d0 install
	xcopy /e /i /y %BOOST_ROOT%\include\boost-1_59\boost %BOOST_ROOT%\boost > nul
	popd
)

if %nocache% == 1 (
	call .\build.bat thirdparty
	date /t > thirdparty.cached & time /t >> thirdparty.cached
	echo.
	echo Thirdparty libraries installed.
	echo.
)

