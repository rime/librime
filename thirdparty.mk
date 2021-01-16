# a minimal build of third party libraries for static linking

third_party_dir = $(CURDIR)/thirdparty
src_dir = $(third_party_dir)/src

glog: build ?= cmake-build
build ?= build

third_party_libs = capnproto glog gtest leveldb marisa opencc yaml-cpp

.PHONY: all clean-src $(third_party_libs)

all: $(third_party_libs)

# note: this won't clean output files under include/, lib/ and bin/.
clean-src:
	rm -r $(src_dir)/capnproto/build || true
	rm -r $(src_dir)/glog/cmake-build || true
	rm -r $(src_dir)/googletest/build || true
	rm -r $(src_dir)/leveldb/build || true
	rm -r $(src_dir)/marisa-trie/build || true
	rm -r $(src_dir)/opencc/build || true
	rm -r $(src_dir)/yaml-cpp/build || true

capnproto:
	cd $(src_dir)/capnproto; \
	cmake . -B$(build) \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	-DBUILD_TESTING:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

glog:
	cd $(src_dir)/glog; \
	cmake . -B$(build) \
	-DBUILD_TESTING:BOOL=OFF \
	-DWITH_GFLAGS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

gtest:
	cd $(src_dir)/googletest; \
	cmake . -B$(build) \
	-DBUILD_GMOCK:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

leveldb:
	cd $(src_dir)/leveldb; \
	cmake . -B$(build) \
	-DLEVELDB_BUILD_BENCHMARKS:BOOL=OFF \
	-DLEVELDB_BUILD_TESTS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

marisa:
	cd $(src_dir)/marisa-trie; \
	cmake $(src_dir) -B$(build) \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

opencc:
	cd $(src_dir)/opencc; \
	cmake . -B$(build) \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install

yaml-cpp:
	cd $(src_dir)/yaml-cpp; \
	cmake . -B$(build) \
	-DYAML_CPP_BUILD_CONTRIB:BOOL=OFF \
	-DYAML_CPP_BUILD_TESTS:BOOL=OFF \
	-DYAML_CPP_BUILD_TOOLS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(third_party_dir)" \
	&& cmake --build $(build) --target install
