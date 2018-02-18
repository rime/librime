Rime with Windows
===

Prerequisites
---
librime is tested to work on Windows with the following build tools and libraries:
  - Visual Studio 2015
  - [Boost](http://www.boost.org/)>=1.60
  - [cmake](http://www.cmake.org/)>=2.8
  
[Python](https://python.org)>=2.7 is needed to build opencc dictionaries.

You may need to update Boost when using a higher version of VS.

You can also build third-party libraries manually, by following instructions in the build script.

Get the code
---
``` batch
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime).

Setup a build environment
---
Copy `env.bat.template` to `env.bat` and edit the script according to your setup.
Specifically, make sure `BOOST_ROOT` is set to the path to Boost source directory;
modify `CMAKE_GENERATOR` and `PLATFORM_TOOLSET` if using a different version of Visual Studio;
set `DEVTOOLS_PATH` for build tools installed to a custom location.

When prepared, run the following commands in a Developer Command Prompt window.

Build Boost
---
``` batch
build.bat boost
```

Build third-party libraries
---
``` batch
build.bat thirdparty
```
This builds dependent libraries in `thirdparty\src\*`, and copies artifacts to `thirdparty\lib` and `thirdparty\bin`.

Build librime
---
``` batch
build.bat librime
```
This creates `build\lib\Release\rime.dll`.

Try it in the console
---
``` batch
copy /Y build\lib\Release\rime.dll build\bin
cd build\bin
echo "congmingdeRime{space}shurufa" | Release\rime_api_console.exe > output.txt
```
