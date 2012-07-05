all: brise librime
	@echo ':)'

brise:
	if [ -e ../brise ]; then cp -R ../brise/* data/; fi

librime:
	mkdir -p build
	(cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_STATIC=OFF ..)
	make -C build

install:
	make -C build install

uninstall:
	make -C build uninstall
