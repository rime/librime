rem Customize your build environment and save the modified copy to env.bat

rem REQUIRED: path to Boost source directory
if not defined BOOST_ROOT (
  rem Requires PowerShell to parse boost-data.txt.
  for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "$data = Get-Content -Raw -Path '%BOOST_DATA_FILE%' ^| ConvertFrom-StringData; if ($data.ContainsKey('version')) { $data.version.Trim() }"`) do if not defined BOOST_VERSION set "BOOST_VERSION=%%I"
  if not defined BOOST_VERSION (
    echo Error: missing version in %BOOST_DATA_FILE%.
    exit /b 1
  )
  if defined BOOST_VERSION set BOOST_ROOT=%RIME_ROOT%\deps\boost-%BOOST_VERSION%
)

rem architecture, Visual Studio version and platform toolset
set ARCH=Win32
set BJAM_TOOLSET=msvc-14.2
set CMAKE_GENERATOR="Visual Studio 16 2019"
set PLATFORM_TOOLSET=v142

rem OPTIONAL: path to additional build tools
rem set DEVTOOLS_PATH=%ProgramFiles%\Git\cmd;%ProgramFiles%\CMake\bin;C:\Python27;
