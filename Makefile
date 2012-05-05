all: librime
	@echo ':)'

librime:
	mkdir -p build
	(cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_STATIC=OFF ..)
	make -C build

install:
	make -C build install
