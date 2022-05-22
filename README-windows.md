# Rime with Windows

## Prerequisites

`librime` is tested to work on Windows with the following combinations of build
tools and libraries:

  - Visual Studio 2017
  - [Boost](http://www.boost.org/)=1.64
  - [cmake](http://www.cmake.org/)>=3.8

and

  - Visual Studio 2015
  - [Boost](http://www.boost.org/)=1.60
  - [cmake](http://www.cmake.org/)>=3.8

Boost and cmake versions need to match higher VS version.

[Python](https://python.org)>=2.7 is needed to build opencc dictionaries.

## Get the code

``` batch
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime).

## Setup a build environment

Copy `env.bat.template` to `env.bat` and edit the file according to your setup.
Specifically, make sure `BOOST_ROOT` is set to the root directory of Boost
source tree; modify `BJAM_TOOLSET`, `CMAKE_GENERATOR` and `PLATFORM_TOOLSET` if
using a different version of Visual Studio; also set `DEVTOOLS_PATH` for build
tools installed to custom location.

When prepared, do the following in a *Developer Command Prompt* window.

## Build Boost

``` batch
build.bat boost
```

## Build third-party libraries

``` batch
build.bat deps
```
This builds dependent libraries in `librime\deps\*`, and copies artifacts to
`librime\include`, `librime\lib` and `librime\bin`.

## Build librime

``` batch
build.bat librime
```
This creates `build\lib\Release\rime.dll`.

Build artifacts - the shared library along with API headers and supporting files
are gathered in `dist` directory.

## Try it in the console

`librime` comes with a REPL application which can be used to test if the library
is working.

``` batch
copy /Y build\lib\Release\rime.dll build\bin
cd build\bin
echo congmingdeRime{space}shurufa | Release\rime_api_console.exe > output.txt
```

Instead of redirecting output to a file, you can set appropriate code page
(`chcp 65001`) and font in the console to work with the REPL interactively.
