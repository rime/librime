RIME_ROOT = $(CURDIR)

sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

.PHONY: all install uninstall thirdparty clean librime-static librime debug

all: librime
	@echo ':)'

install: install-librime
	@echo ':)'

uninstall: uninstall-librime
	@echo ':)'

thirdparty:
	make -f Makefile.thirdparty

thirdparty/%:
	make -f Makefile.thirdparty $(@:thirdparty/%=%)

clean:
	rm -Rf build build-static debug-build

librime-static:
	cmake . -Bbuild-static -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON -DBUILD_SHARED_LIBS=OFF
	cmake --build build-static

librime:
	cmake . -Bbuild -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
	cmake --build build

install-librime:
	cmake --build build --target install

uninstall-librime:
	cmake --build build --target uninstall

debug:
	cmake . -Bdebug-build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DBUILD_TEST=ON
	cmake --build debug-build

install-debug:
	cmake --build debug-build --target install

uninstall-debug:
	cmake --build debug-build --target uninstall
