@echo off
if exist env.bat call env.bat
set OLD_PATH=%PATH%
if defined DEV_PATH set PATH=%DEV_PATH%;%OLD_PATH%
cmd
