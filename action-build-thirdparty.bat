setlocal

set nocache=0

if not exist thirdparty.cached set nocache=1

git submodule status > thirdparty.submodule-status.new
if not exist thirdparty.submodule-status (
  set nocache=1
) else (
  fc thirdparty.submodule-status thirdparty.submodule-status.new
  if errorlevel 1 set nocache=1
)

if %nocache% == 1 (
	git submodule update --init
	call .\build.bat thirdparty
	if errorlevel 1 goto error

	copy /y thirdparty.submodule-status.new thirdparty.submodule-status
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

goto exit

:error
set exitcode=%errorlevel%

:exit
exit /b %exitcode%
