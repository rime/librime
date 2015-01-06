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

You can also manually download and build Boost libraries from source code, then set `BOOST_ROOT` to the path of its top level directory prior to building librime.

Get the code
---
``` sh
git clone git@github.com:lotem/librime.git
```
or [download from github](https://github.com/lotem/librime).

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
