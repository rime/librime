setlocal

set boost_build_options=toolset=msvc-14.1^
 variant=release^
 link=static^
 threading=multi^
 runtime-link=static^
 define=BOOST_USE_WINAPI_VERSION=0x0501^
 cxxflags="/Zc:threadSafeInit- "^
 --with-date_time^
 --with-filesystem^
 --with-locale^
 --with-regex^
 --with-system^
 --with-thread

set nocache=0

if not exist boost.cached set nocache=1
if not exist %BOOST_ROOT%\stage set nocache=1

if %nocache% == 1 (
	pushd %BOOST_ROOT%
	call .\bootstrap.bat
	.\b2.exe %boost_build_options% -q -d0 stage
	popd
	if %ERRORLEVEL% NEQ 0 goto ERROR

	date /t > boost.cached & time /t >> boost.cached
	echo.
	echo Boost libraries installed.
	echo.
) else (
	echo.
	echo Last build date of Boost cache is
	type boost.cached
	echo.
)

goto EXIT

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
