Rime with Windows
===

Prerequisites
---
librime is tested to work on Windows with the following build tools and libraries:
  - VS2013
  - [Boost](http://www.boost.org/)>=1.56
  - [cmake](http://www.cmake.org/)>=2.8
  - [msysgit](https://msysgit.github.io/)

You may need to update Boost when using a higher version of VS.

The following command line tools from msysgit are needed by `build.bat` to automate the build process:
  - **git**, used by `build.bat thirdparty` to clone `windows` branch from [lotem/leveldb](https://github.com/lotem/leveldb)
  - **sed**, used by `build.bat thirdparty` to edit project files for static-linking C++ runtime (/MT)

You can also build third-party libraries manually without them, by following instructions in the build script.

Get the code
---
``` batch
git clone git@github.com:lotem/librime.git
```
or [download from github](https://github.com/lotem/librime).

Setup a build environment
---
Copy `env.bat` from `env.bat.template` and edit the script according to your setup.
Specifically, make sure `BOOST_ROOT` is set to the path where you extracted Boost source;
modify `*_INSTALL_PATH` if you've installed build tools in a custom location.

When finished, run `shell.bat` to complete the following steps in a prepared command prompt.

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
build.bat
```
This creates `build\lib\Release\rime.dll`.

Try it in the console
---
``` batch
copy /Y build\lib\Release\rime.dll build\bin
copy /Y thirdparty\bin\*.dll build\bin
cd build\bin
echo "congmingdeRime{space}shurufa" | Release\rime_api_console > output.txt
```
