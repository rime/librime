setlocal

if not defined RIME_ROOT set RIME_ROOT=%CD%

rem for GitHub pull request #1, git checkout 1/merge
set clone_options=^
 --config "remote.origin.fetch=+refs/pull/*:refs/remotes/origin/*" ^
 --depth 1 ^
 --no-single-branch

echo RIME_PLUGINS=%RIME_PLUGINS% > version-info.txt
echo librime >> version-info.txt
git describe --always >> version-info.txt

if defined RIME_PLUGINS (
  for %%s in (%RIME_PLUGINS%) do call :install_plugin %%s || exit /b
)
exit /b

:install_plugin
set plugin=%1
echo plugin: %plugin%
for /f "delims=@" %%i in ("%plugin%") do (
  set slug=%%i
  goto :got_slug
)
:got_slug
if %slug% == %plugin% (
  set branch=
) else (
  set branch=%plugin:*@=%
)
set plugin_project=%slug:*/=%
set plugin_dir=plugins\%plugin_project:librime-=%
git clone %clone_options% "https://github.com/%slug%.git" %plugin_dir%
if errorlevel 1 exit /b
rem pull request ref doesn't work with git clone --branch
if not [%branch%] == [] (
   git -C %plugin_dir% checkout %branch%
   if errorlevel 1 exit /b
)
:action_install_plugin
echo %plugin% >> version-info.txt
git -C %plugin_dir% describe --always >> version-info.txt
if exist %plugin_dir%\action-install.bat (
  pushd %plugin_dir%
  call action-install.bat
  if errorlevel 1 exit /b
  popd
)
exit /b
