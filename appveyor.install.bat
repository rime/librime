setlocal

set boost_build_options=toolset=msvc-14.0^
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
if not exist %BOOST_ROOT% set nocache=1

if %nocache% == 1 (
	pushd C:\Libraries
	appveyor DownloadFile https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.7z
	7z x boost_1_68_0.7z | find "ing archive"
	cd boost_1_68_0
	call .\bootstrap.bat
	call .\b2.exe --prefix=%BOOST_ROOT% %boost_build_options% -q -d0 install
	xcopy /e /i /y /q %BOOST_ROOT%\include\boost-1_68\boost %BOOST_ROOT%\boost
	popd
	if %ERRORLEVEL% NEQ 0 goto ERROR

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
