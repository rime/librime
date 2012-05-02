all: librime
	@echo ':)'

librime:
	mkdir -p build
	(cd build; cmake -DBUILD_STATIC=OFF ..)
	make -C build

install:
	make -C build install
