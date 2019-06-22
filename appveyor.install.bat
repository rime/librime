setlocal

call appveyor_build_boost.bat
if %ERRORLEVEL% NEQ 0 goto ERROR

call appveyor_build_thirdparty.bat
if %ERRORLEVEL% NEQ 0 goto ERROR

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
