sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

all: librime
	@echo ':)'

install: install-librime
	@echo ':)'

uninstall: uninstall-librime
	@echo ':)'

librime-static:
	mkdir -p build-static
	(cd build-static; cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON ..)
	make -C build-static

librime:
	mkdir -p build
	(cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..)
	make -C build

install-librime:
	make -C build install

uninstall-librime:
	make -C build uninstall

debug:
	mkdir -p debug-build
	(cd debug-build; cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..)
	make -C debug-build

debug-install:
	make -C debug-build install

debug-uninstall:
	make -C debug-build uninstall
