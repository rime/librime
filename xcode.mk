RIME_ROOT = $(CURDIR)

dist_dir = $(RIME_ROOT)/dist

ifdef BOOST_ROOT
CMAKE_BOOST_OPTIONS = -DBoost_NO_BOOST_CMAKE=TRUE \
	-DBOOST_ROOT="$(BOOST_ROOT)"
endif

# https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_SYSROOT.html
export SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path)

# https://cmake.org/cmake/help/latest/envvar/MACOSX_DEPLOYMENT_TARGET.html
export MACOSX_DEPLOYMENT_TARGET ?= 10.13

ifdef BUILD_UNIVERSAL
# https://cmake.org/cmake/help/latest/envvar/CMAKE_OSX_ARCHITECTURES.html
export CMAKE_OSX_ARCHITECTURES = arm64;x86_64
endif

# boost::locale library from homebrew links to homebrewed icu4c libraries
icu_prefix = $(shell brew --prefix)/opt/icu4c

debug debug-with-icu test-debug: build ?= debug
build ?= build

.PHONY: all release debug clean dist distclean test test-debug deps thirdparty \
release-with-icu debug-with-icu dist-with-icu

all: release

release:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(dist_dir)" \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Release

release-with-icu:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_WITH_ICU=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(dist_dir)" \
	-DCMAKE_PREFIX_PATH="$(icu_prefix)" \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Release

debug:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_SEPARATE_LIBS=ON \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Debug

debug-with-icu:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_SEPARATE_LIBS=ON \
	-DBUILD_WITH_ICU=ON \
	-DCMAKE_PREFIX_PATH="$(icu_prefix)" \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Debug

clean:
	rm -rf build > /dev/null 2>&1 || true
	rm -rf debug > /dev/null 2>&1 || true
	rm build.log > /dev/null 2>&1 || true
	rm -f lib/* > /dev/null 2>&1 || true
	$(MAKE) -f deps.mk clean-src

dist: release
	cmake --build $(build) --config Release --target install

dist-with-icu: release-with-icu
	cmake --build $(build) --config Release --target install

distclean: clean
	rm -rf "$(dist_dir)" > /dev/null 2>&1 || true

test: release
	(cd $(build)/test; DYLD_LIBRARY_PATH=../lib/Release Release/rime_test)

test-debug: debug
	(cd $(build)/test; Debug/rime_test)

# `thirdparty` is deprecated in favor of `deps`
deps thirdparty:
	$(MAKE) -f deps.mk

deps/boost thirdparty/boost:
	./install-boost.sh

deps/%:
	$(MAKE) -f deps.mk $(@:deps/%=%)

thirdparty/%:
	$(MAKE) -f deps.mk $(@:thirdparty/%=%)
