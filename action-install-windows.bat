setlocal
set boost_version=1_76_0
aria2c https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_%boost_version%.7z -d C:\Libraries
pushd C:\Libraries
7z x boost_%boost_version%.7z
popd C:\Libraries
set BOOST_ROOT=C:\Libraries\boost_%boost_version%

call build.bat boost
if errorlevel 1 goto error

call build.bat thirdparty
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
if exist %plugin_dir%\action.install.bat (
  call %plugin_dir%\action.install.bat
)
exit /b
