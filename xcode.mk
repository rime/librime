RIME_ROOT = $(CURDIR)

RIME_DIST_DIR = $(RIME_ROOT)/dist

ifdef BOOST_ROOT
CMAKE_BOOST_OPTIONS = -DBoost_NO_BOOST_CMAKE=TRUE \
	-DBOOST_ROOT="$(BOOST_ROOT)"
endif

# https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_SYSROOT.html
export SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path)

# https://cmake.org/cmake/help/latest/envvar/MACOSX_DEPLOYMENT_TARGET.html
export MACOSX_DEPLOYMENT_TARGET ?= 10.9

ifdef BUILD_UNIVERSAL
# https://cmake.org/cmake/help/latest/envvar/CMAKE_OSX_ARCHITECTURES.html
export CMAKE_OSX_ARCHITECTURES = arm64;x86_64
endif

ICU_PREFIX = $(shell brew --prefix)/opt/icu4c

build = build
debug debug-with-icu test-debug: build = debug

.PHONY: all release debug clean distclean test test-debug thirdparty \
release-with-icu debug-with-icu dist-with-icu

all: release

release:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(RIME_DIST_DIR)" \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Release

release-with-icu:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_WITH_ICU=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(RIME_DIST_DIR)" \
	-DCMAKE_PREFIX_PATH="$(ICU_PREFIX)" \
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
	-DCMAKE_PREFIX_PATH="$(ICU_PREFIX)" \
	$(CMAKE_BOOST_OPTIONS)
	cmake --build $(build) --config Debug

clean:
	rm -rf build > /dev/null 2>&1 || true
	rm -rf debug > /dev/null 2>&1 || true
	rm build.log > /dev/null 2>&1 || true
	rm -f thirdparty/lib/* > /dev/null 2>&1 || true
	make -f thirdparty.mk clean-src

dist: release
	cmake --build $(build) --config Release --target install

dist-with-icu: release-with-icu
	cmake --build $(build) --config Release --target install

distclean: clean
	rm -rf "$(RIME_DIST_DIR)" > /dev/null 2>&1 || true

test: release
	(cd $(build)/test; LD_LIBRARY_PATH=../lib/Release Release/rime_test)

test-debug: debug
	(cd $(build)/test; Debug/rime_test)

thirdparty:
	make -f thirdparty.mk

thirdparty/boost:
	./install-boost.sh

thirdparty/%:
	make -f thirdparty.mk $(@:thirdparty/%=%)
