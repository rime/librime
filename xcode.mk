RIME_ROOT = $(CURDIR)

RIME_DIST_DIR = $(RIME_ROOT)/dist

RIME_COMPILER_OPTIONS = CC=clang CXX=clang++ \
CXXFLAGS="-stdlib=libc++" LDFLAGS="-stdlib=libc++"

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
	-DCMAKE_INSTALL_PREFIX="$(RIME_DIST_DIR)"
	cmake --build $(build) --config Release

release-with-icu:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(RIME_DIST_DIR)" \
	-DBUILD_WITH_ICU=ON \
	-DCMAKE_PREFIX_PATH=$(ICU_PREFIX)
	cmake --build $(build) --config Release

debug:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_SEPARATE_LIBS=ON
	cmake --build $(build) --config Debug

debug-with-icu:
	cmake . -B$(build) -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_SEPARATE_LIBS=ON \
	-DBUILD_WITH_ICU=ON \
	-DCMAKE_PREFIX_PATH=$(ICU_PREFIX)
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
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk

thirdparty/%:
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk $(@:thirdparty/%=%)
