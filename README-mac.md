# Rime with Mac

## Preparation

Install Xcode with command line tools.

Install other build tools:

``` sh
brew install cmake git
```

Install Boost C++ libraries:

``` sh
brew install boost
```

> **Note:**
>
> Starting from version 1.68, `boost::locale` library from Homebrew depends on
> `icu4c`, which is not provided by macOS.
>
> The make target `xcode/release-with-icu` tells cmake to link to ICU libraries
>  installed locally with Homebrew. This is only required if building with the
> `librime-charcode` plugin.
>
> To make a portable build with the plugin, install an earlier version of
> `boost` via homebrew:

``` sh
brew install boost@1.60
brew link --force boost@1.60
```

If you manually download and build Boost libraries from source code, set shell
variable `BOOST_ROOT` to its top level directory prior to building librime.

## Get the code

``` sh
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime), then get code for
third party dependencies separately.

## Build third-party libraries

``` sh
cd librime
make xcode/thirdparty
```

This builds dependent libraries in `thirdparty/src/*`, and copies artifacts to
`thirdparty/lib` and `thirdparty/bin`.

You can build an individual library, eg. opencc, with:

``` sh
make xcode/thirdparty/opencc
```

## Build librime

``` sh
make xcode
```
This creates `build/lib/Release/librime.dylib` and command line tools
`build/bin/Release/rime_*`.

Or, create a debug build:

``` sh
make xcode/debug
```

## Run unit tests

``` sh
make xcode/test
```

Or, test the debug build:

``` sh
make xcode/test-debug
```

## Try it in the console

``` sh
(
  cd debug/bin;
  echo "congmingdeRime{space}shurufa" | Debug/rime_api_console
)
```

Use it as REPL, quit with <kbd>Control+d</kbd>:

``` sh
(cd debug/bin; ./Debug/rime_api_console)
```
