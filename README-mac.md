# Rime with Mac

## Preparation

Install Xcode with command line tools.

Install other build tools:

``` sh
brew install cmake git
```

## Get the code

``` sh
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime), then get code for
third-party dependencies separately.

## Install Boost C++ libraries

Boost is a third-party library which librime code heavily depend on.
These dependencies include a few header-only Boost libraries.

**Option 1 (recommended):** Download and build Boost from source.

``` sh
cd librime
bash install-boost.sh
```

The make script will download Boost source tarball, extract it to
`librime/deps/boost-<version>`.

Set shell variable `BOOST_ROOT` to the path to `boost-<version>` directory prior
to building librime.

``` sh
export BOOST_ROOT="$(pwd)/deps/boost-1.84.0"
```

**Option 2:** Install Boost libraries from Homebrew.

``` sh
brew install boost
# to build with icu4c, add the icu4c install path to LIBRARY_PATH
export LIBRARY_PATH=${LIBRARY_PATH}:/opt/homebrew/opt/icu4c/lib:/usr/local/opt/icu4c/lib
```

This is a time-saving option if you are building and installing Rime only for your
own Mac computer.

Built with Homebrewed version of Boost, the `librime` binary will not be
portable to machines without certain Homebrew formulae installed.

**Option 3:** Install an older version of Boost libraries from Homebrew.

Starting from version 1.68, `boost::locale` library from Homebrew depends on
`icu4c`, which is not provided by macOS.

Make target `xcode/release-with-icu` tells cmake to link to ICU libraries
installed locally with Homebrew. This is only required if building with the
[`librime-charcode`](https://github.com/rime/librime-charcode) plugin.

To make a portable build with this plugin, install an earlier version of
`boost` that wasn't dependent on `icu4c`:

``` sh
brew install boost@1.60
brew link --force boost@1.60
```

## Build third-party libraries

Required third-party libraries other than Boost are included as git submodules:

``` sh
# cd librime

# if you didn't checked out the submodules with git clone --recursive, now do:
# git submodule update --init

make deps
```

This builds libraries located at `librime/deps/*`, and installs the build
artifacts to `librime/include`, `librime/lib` and `librime/bin`.

You can also build an individual library, eg. `opencc`, with:

``` sh
make deps/opencc
```

## Build librime

``` sh
make
```
This creates `build/lib/Release/librime*.dylib` and command line tools
`build/bin/Release/rime_*`.

Or, create a debug build:

``` sh
make debug
```

## Run unit tests

``` sh
make test
```

Or, test the debug build:

``` sh
make test-debug
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
