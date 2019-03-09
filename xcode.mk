RIME_ROOT = $(CURDIR)

RIME_COMPILER_OPTIONS = CC=clang CXX=clang++ \
CXXFLAGS="-stdlib=libc++" LDFLAGS="-stdlib=libc++"

.PHONY: all release debug clean test thirdparty

all: release

release:
	cmake . -Bbuild -GXcode \
	-DBUILD_STATIC=ON \
	-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
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

test: release
	(cd build/test; LD_LIBRARY_PATH=../lib/Release Release/rime_test)

test-debug: debug
	(cd debug/test; Debug/rime_test)

thirdparty:
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk

thirdparty/%:
	$(RIME_COMPILER_OPTIONS) make -f thirdparty.mk $(@:thirdparty/%=%)
