sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

all: librime
	@echo ':)'

install: install-librime
	@echo ':)'

uninstall: uninstall-librime
	@echo ':)'

librime:
	mkdir -p build
	(cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr ..)
	make -C build

install-librime:
	make -C build install

uninstall-librime:
	make -C build uninstall
