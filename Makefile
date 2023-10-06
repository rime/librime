RIME_ROOT ?= $(CURDIR)

ifeq ($(shell uname),Darwin) # for macOS
prefix ?= $(RIME_ROOT)/dist

ifdef BOOST_ROOT
CMAKE_BOOST_OPTIONS = -DBoost_NO_BOOST_CMAKE=TRUE \
	-DBOOST_ROOT="$(BOOST_ROOT)"
endif

# https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_SYSROOT.html
export SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path)

# https://cmake.org/cmake/help/latest/envvar/MACOSX_DEPLOYMENT_TARGET.html
export MACOSX_DEPLOYMENT_TARGET ?= 10.15

ifdef BUILD_UNIVERSAL
# https://cmake.org/cmake/help/latest/envvar/CMAKE_OSX_ARCHITECTURES.html
export CMAKE_OSX_ARCHITECTURES = arm64;x86_64
endif

# boost::locale library from homebrew links to homebrewed icu4c libraries
icu_prefix = $(shell brew --prefix)/opt/icu4c

else # for Linux
prefix ?= $(DESTDIR)/usr
endif

ifndef NOPARALLEL
export MAKEFLAGS+=" -j$(( $(nproc) + 1)) "
endif

debug install-debug uninstall-debug test-debug: build ?= debug
build ?= build

.PHONY: all deps thirdparty xcode clean \
librime librime-static install-librime uninstall-librime \
release debug test install uninstall install-debug uninstall-debug

all: release

# `thirdparty` is deprecated in favor of `deps`
deps thirdparty:
	$(MAKE) -f deps.mk

deps/%:
	$(MAKE) -f deps.mk $(@:deps/%=%)

thirdparty/%:
	$(MAKE) -f deps.mk $(@:thirdparty/%=%)

clean:
	rm -Rf build debug

librime: release
install-librime: install
uninstall-librime: uninstall

librime-static:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_STATIC=ON \
	-DBUILD_SHARED_LIBS=OFF
	cmake --build $(build)

release:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=OFF \
	-DENABLE_EXTERNAL_PLUGINS=ON
	cmake --build $(build)

merged-plugins:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=ON \
	-DENABLE_EXTERNAL_PLUGINS=OFF
	cmake --build $(build)

debug:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Debug \
	-DBUILD_MERGED_PLUGINS=OFF \
	-DENABLE_EXTERNAL_PLUGINS=ON
	cmake --build $(build)

install:
	cmake --build $(build) --target install

install-debug:
	cmake --build $(build) --target install

uninstall:
	cmake --build $(build) --target remove

uninstall-debug:
	cmake --build $(build) --target remove

test: release
	(cd $(build); ctest --output-on-failure)

test-debug: debug
	(cd $(build); ctest --output-on-failure)
