RIME_ROOT = $(CURDIR)

RIME_DIST_DIR = $(RIME_ROOT)/dist

RIME_COMPILER_OPTIONS = CC=clang CXX=clang++ \
CXXFLAGS="-stdlib=libc++" LDFLAGS="-stdlib=libc++"

.PHONY: all release debug clean distclean test thirdparty

all: release

release:
	cmake . -Bbuild -GXcode \
	-DBUILD_STATIC=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
	-DCMAKE_INSTALL_PREFIX="$(RIME_DIST_DIR)"
	cmake --build build --config Release

debug:
	cmake . -Bdebug -GXcode \
	-DBUILD_STATIC=ON \
	-DBUILD_SEPARATE_LIBS=ON
	cmake --build debug --config Debug

clean:
	rm -rf build > /dev/null 2>&1 || true
	rm -rf debug > /dev/null 2>&1 || true
	rm build.log > /dev/null 2>&1 || true
	rm -f thirdparty/lib/* > /dev/null 2>&1 || true
	make -f thirdparty.mk clean-src

dist: release
	cmake --build build --config Release --target install

distclean: clean
	rm -rf "$(RIME_DIST)" > /dev/null 2>&1 || true

test: release
	(cd build/test; LD_LIBRARY_PATH=../lib/Release Release/rime_test)

test-debug: debug
	(cd debug/test; Debug/rime_test)

thirdparty:
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk

thirdparty/%:
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk $(@:thirdparty/%=%)
