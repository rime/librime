sharedir = $(DESTDIR)/usr/share
bindir = $(DESTDIR)/usr/bin

all: brise librime
	@echo ':)'

brise:
	if [ -e ../brise ]; then cp -R ../brise/* data/; fi

librime:
	mkdir -p build
	(cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_STATIC=OFF ..)
	make -C build

install-precompiled-data:
	@echo 'precompiling Rime schemas, patience...'
	$(bindir)/rime_deployer --build $(sharedir)/rime-data
	if [ -e $(sharedir)/rime-data/rime.log ]; then rm $(sharedir)/rime-data/rime.log; fi

uninstall-precompiled-data:
	rm $(sharedir)/rime-data/*.bin || true

install-librime:
	make -C build install

uninstall-librime:
	make -C build uninstall

install: install-librime install-precompiled-data
	@echo ':)'

uninstall: uninstall-precompiled-data uninstall-librime
	@echo ':)'
