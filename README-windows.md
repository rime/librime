# Rime with Windows

## Prerequisites

`librime` is tested to work on Windows with the following combinations of build
tools and libraries:

  - Visual Studio 2022 or LLVM 16
  - [Boost](http://www.boost.org/)>=1.83
  - [cmake](http://www.cmake.org/)>=3.10

Boost and cmake versions need to match higher VS version.

[Python](https://python.org)>=2.7 is needed to build opencc dictionaries.

## Get the code

``` batch
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime).

## Setup a build environment

Copy `env.bat.template` to `env.bat` and edit the file according to your setup.
If Boost libraries are available, set `BOOST_ROOT` to the root directory of
Boost source tree; modify `BJAM_TOOLSET`, `CMAKE_GENERATOR` and
`PLATFORM_TOOLSET` if using a different version of Visual Studio; also set
`DEVTOOLS_PATH` for build tools installed to custom location.

When prepared, do the following in a *Developer Command Prompt* window.

## Install Boost

This step downloads Boost libraries in librime's default search path.
If you have installed Boost libraries elsewhere, skip this step and set
the environment varialble `BOOST_ROOT` to the installed path.

``` batch
install-boost.bat
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
This creates `build\bin\Release\rime.dll`.

Build artifacts: the shared library along with API headers and supporting files
can be found in `dist` directory.

## Try it in the console

`librime` comes with a REPL application which can be used to test if the library
is working.

``` batch
cd build\bin
Release\rime_api_console.exe
congmingdeRime shurufa
```
