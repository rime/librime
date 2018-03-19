RIME_ROOT = $(CURDIR)/..

sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

.PHONY: release debug clean install uninstall

release:
	cmake . -Bbuild -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
	cmake --build build

debug:
	cmake . -Bdebug-build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug
	cmake --build debug-build

clean:
	rm -Rf build

install:
	cmake --build build --target install

install-debug:
	cmake --build debug-build --target install
