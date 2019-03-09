RIME_ROOT = $(CURDIR)

sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

.PHONY: all thirdparty xcode clean\
librime librime-static install-librime uninstall-librime \
release debug test install uninstall install-debug uninstall-debug

all: release

thirdparty:
	make -f thirdparty.mk

thirdparty/%:
	make -f thirdparty.mk $(@:thirdparty/%=%)

xcode:
	make -f xcode.mk

xcode/%:
	make -f xcode.mk $(@:xcode/%=%)

clean:
	rm -Rf build build-static debug

librime: release
install-librime: install
uninstall-librime: uninstall

librime-static:
	cmake . -Bbuild-static \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_STATIC=ON \
	-DBUILD_SHARED_LIBS=OFF
	cmake --build build-static

release:
	cmake . -Bbuild \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=OFF
	cmake --build build

merged-plugins:
	cmake . -Bbuild \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=ON
	cmake --build build

debug:
	cmake . -Bdebug \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Debug
	cmake --build debug

install:
	cmake --build build --target install

install-debug:
	cmake --build debug --target install

uninstall:
	cmake --build build --target uninstall

uninstall-debug:
	cmake --build debug --target uninstall

test: release
	(cd build/test; ./rime_test)

test-debug: debug
	(cd debug/test; ./rime_test)
