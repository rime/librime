RIME_ROOT ?= $(CURDIR)

RIME_SOURCE_PATH = plugins sample src test tools

OS_NAME = $(shell uname)
ifeq ($(OS_NAME),Darwin) # for macOS
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
else ifeq ($(shell test -n "$$PREFIX" && echo "$$PREFIX" | grep -q "/data/data/com.termux" && echo "termux"),termux)
prefix ?= $(PREFIX)
else ifeq ($(OS_NAME),FreeBSD)
prefix ?= $(DESTDIR)/usr/local
else ifeq ($(OS_NAME),OpenBSD)
prefix ?= $(DESTDIR)/usr/local
else # for Linux
prefix ?= $(DESTDIR)/usr
endif

ifndef NOPARALLEL
export MAKEFLAGS+=" -j$$(( $$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 8) + 1)) "
endif

debug install-debug uninstall-debug test-debug: build ?= debug
build ?= build

.PHONY: all deps clean \
librime librime-static \
release debug test install uninstall \
install-debug uninstall-debug

all: release

clang-format-lint:
	find ${RIME_SOURCE_PATH} \! -path 'plugins/*/*' -a \( -name '*.cc' -o -name '*.h' \) | \
	xargs clang-format -Werror --dry-run || { echo Please lint your code by '"'"make clang-format-apply"'"'.; false; }

clang-format-apply:
	find ${RIME_SOURCE_PATH} \! -path 'plugins/*/*' -a \( -name '*.cc' -o -name '*.h' \) | xargs clang-format --verbose -i

deps:
	$(MAKE) -f deps.mk

deps/%:
	$(MAKE) -f deps.mk $(@:deps/%=%)

clean:
	rm -r $(build) || true

librime: release

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
	-DALSO_LOG_TO_STDERR=ON \
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
