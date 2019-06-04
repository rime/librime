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

if not exist thirdparty.cached set nocache=1
if not exist %BOOST_ROOT%\stage set nocache=1

if %nocache% == 1 (
	pushd %BOOST_ROOT%
	call .\bootstrap.bat
	.\b2.exe %boost_build_options% -q -d0 stage
	popd
	if %ERRORLEVEL% NEQ 0 goto ERROR

	git submodule update --init
	call .\build.bat thirdparty
	if %ERRORLEVEL% NEQ 0 goto ERROR

	date /t > thirdparty.cached & time /t >> thirdparty.cached
	echo.
	echo Thirdparty libraries installed.
	echo.
) else (
	echo.
	echo Last build date of cache is
	type thirdparty.cached
	echo.
)

if defined RIME_PLUGINS (
   for %%s in (%RIME_PLUGINS%) do call :install_plugin %%s
)

goto EXIT

:ERROR
set EXITCODE=%ERRORLEVEL%

:EXIT
exit /b %EXITCODE%

:install_plugin
set slug=%1
echo "plugin: %slug%"
set plugin_project=%slug:*/=%
set plugin_dir=plugins/%plugin_project:librime-=%
git clone --depth 1 "https://github.com/%slug%.git" %plugin_dir%
if exist %plugin_dir%\appveyor.install.bat (
  call %plugin_dir%\appveyor.install.bat
)
exit /b
