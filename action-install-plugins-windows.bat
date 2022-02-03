setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%

if defined RIME_PLUGINS (
   for %%s in (%RIME_PLUGINS%) do call :install_plugin %%s || exit /b
)
exit /b

:install_plugin
set slug=%1
echo "plugin: %slug%"
set plugin_project=%slug:*/=%
set plugin_dir=plugins/%plugin_project:librime-=%
git clone --depth 1 "https://github.com/%slug%.git" %plugin_dir%
if errorlevel 1 exit /b
if exist %plugin_dir%\action-install.bat (
  call %plugin_dir%\action-install.bat
)
exit /b
