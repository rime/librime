RIME_ROOT ?= $(CURDIR)

prefix ?= $(DESTDIR)/usr

debug install-debug uninstall-debug test-debug: build ?= debug
build ?= build

.PHONY: all thirdparty xcode clean \
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
	rm -Rf build debug

librime: release
install-librime: install
uninstall-librime: uninstall

librime-static:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_STATIC=ON \
	-DBUILD_SHARED_LIBS=OFF
	cmake --build $(build)

release:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=OFF
	cmake --build $(build)

merged-plugins:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_MERGED_PLUGINS=ON
	cmake --build $(build)

debug:
	cmake . -B$(build) \
	-DCMAKE_INSTALL_PREFIX=$(prefix) \
	-DCMAKE_BUILD_TYPE=Debug \
	-DBUILD_MERGED_PLUGINS=OFF
	cmake --build $(build)

install:
	cmake --build $(build) --target install

install-debug:
	cmake --build $(build) --target install

uninstall:
	cmake --build $(build) --target uninstall

uninstall-debug:
	cmake --build $(build) --target uninstall

test: release
	(cd $(build)/test; ./rime_test)

test-debug: debug
	(cd $(build)/test; ./rime_test)
