setlocal

call appveyor_build_boost.bat
if errorlevel 1 goto error

call appveyor_build_thirdparty.bat
if errorlevel 1 goto error

if defined RIME_PLUGINS (
   for %%s in (%RIME_PLUGINS%) do call :install_plugin %%s
)

goto exit

:error
set exitcode=%errorlevel%

:exit
exit /b %exitcode%

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
