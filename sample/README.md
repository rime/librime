# A sample Rime plugin module

This directory offers a Rime plugin named `rime-sample`.

## Overview

Here's how a Rime plugin works:

A *plugin* can either be linked to librime or be a separate shared library,
corresponding to the `BUILD_MERGED_PLUGINS` cmake option.

When the shared library is loaded, it registers itself as *module* `sample`.
If otherwise it's built-in, the module will be automatically loaded by default.

When Rime `initialize`s via API, it loads its default modules or modules
specified by the caller in `RimeTraits::modules`.

Modules can also be loaded on demand using C++ API `rime::LoadModules()`.

When the module is loaded, the `rime_sample_initialize()` function is run,
which registers a *component* `trivial_translator`.

That component is now available for prescription in Rime schema.
It works with the Rime engine in the same way as the built-in translators.

## Build the sample plugin library

``` shell
cd librime
cmake . -Bbuild -DBUILD_SAMPLE=ON -DBUILD_SEPARATE_LIBS=ON
cmake --build build --target rime-sample
```

This outputs shared library: `build/sample/lib/rime-sample.so`

The `BUILD_SEPARATE_LIBS=ON` option is not required but builds faster because
`rime-sample` only depends on the core module of librime.

## Run unit tests

``` shell
cmake --build build --target sample_test

# run tests
build/sample/test/sample_test
```

## Play with sample_console

`trivial_translator` converts pinyin to Chinese numbers.
A sample Rime schema is set up in `build/bin/sample.schema.yaml` to utilize
the translator.

Build the console app and try it with a random number in pinyin:

``` shell
cmake --build build --target sample_console

cd build/sample/bin
echo "yibaiershisanwansiqianlingwushiliu" | ./sample_console
```

## Build as standard Rime plugin

Unlike the sample, which is built after specific rules in librime's cmake
script, standard Rime plugins are separate projects that automatically
integrate into librime's build system, without having to modify any source code
and build script.

To build the sample plugin as standard Rime plugin, link or copy the source code
directory to `plugins/sample` and turn off cmake flag `BUILD_SAMPLE=OFF`.

The cmake option `BUILD_MERGED_PLUGINS` merges all detected plugins into the
built `rime` library. Set the option off to build each plugin as a standalone
(shared) library. In the latter case, the user needs to explicitly load the
`rime-sample` library and load the `sample` module when `initialize`-ing.
