setlocal

set nocache=0

if not exist thirdparty.cached set nocache=1

if %nocache% == 1 (
	git submodule update --init
	call .\build.bat thirdparty
	if %ERRORLEVEL% NEQ 0 goto ERROR

	date /t > thirdparty.cached & time /t >> thirdparty.cached
	echo.
	echo Thirdparty libraries installed.
	echo.
) else (
	echo.
	echo Last build date of thirdparty cache is
	type thirdparty.cached
	echo.
)

goto EXIT

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%
