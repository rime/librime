Rime with Mac
===

Preparation
---

Install Xcode with command line tools.

Install other build tools:
``` sh
brew install cmake
brew install git
```

Install Boost:
``` sh
brew install boost
```

> **Notice:**
>
> Starting from version 1.68, homebrewed `boost` libraries depends on `icu4c`,
> which is not provided by macOS.
>
> The [`with-icu` branch](https://github.com/rime/librime/tree/with-icu) adds
> support for linking to ICU libraries but the built app cannot run on machines
> without ICU libraries installed.
>
> To make the build portable, either install an earlier version of `boost` via
> homebrew, or build from source with bootstrap option `--without-icu`.

When you manually download and build Boost libraries from source code, set shell
variable `BOOST_ROOT` to its top level directory prior to building librime.

Get the code
---
``` sh
git clone --recursive https://github.com/rime/librime.git
```
or [download from GitHub](https://github.com/rime/librime).

Build third-party libraries
---
``` sh
cd librime
make -f Makefile.xcode thirdparty
```
This builds dependent libraries in `thirdparty/src/*`, and copies artifacts to `thirdparty/lib` and `thirdparty/bin`.

You can build an individual library, eg. opencc, with:
``` sh
make -f Makefile.xcode thirdparty/opencc
```

Build librime
---
``` sh
make -f Makefile.xcode
```
This creates `xbuild/lib/Release/librime.dylib` and command line tools `xbuild/bin/Release/rime_*`.

Or, make a debug build. This also creates a test in `xdebug/test/`.
``` sh
make -f Makefile.xcode debug
# run the test
(cd xdebug/test; ./Debug/rime_test)
```

Try it in the console
---
``` sh
(cd xdebug/bin; echo "congmingdeRime{space}shurufa" | Debug/rime_api_console)

# REPL, quit with Control+d
(cd xdebug/bin; ./Debug/rime_api_console)
```
