# a minimal build of third party libraries for static linking

# assuming the Makefile is called from its directory using GNU Make
THIRD_PARTY_DIR = $(CURDIR)/thirdparty
SRC_DIR = $(THIRD_PARTY_DIR)/src
INCLUDE_DIR = $(THIRD_PARTY_DIR)/include
LIB_DIR = $(THIRD_PARTY_DIR)/lib
BIN_DIR = $(THIRD_PARTY_DIR)/bin
DATA_DIR = $(THIRD_PARTY_DIR)/data

THIRD_PARTY_LIBS = glog leveldb marisa opencc yaml-cpp gtest

.PHONY: all clean-src $(THIRD_PARTY_LIBS)

all: $(THIRD_PARTY_LIBS)

# note: this won't clean output files under include/, lib/ and bin/.
clean-src:
	rm -r $(SRC_DIR)/glog/cmake-build || true
	rm -r $(SRC_DIR)/gtest/build || true
	cd $(SRC_DIR)/leveldb; make clean || true
	cd $(SRC_DIR)/marisa-trie; make distclean || true
	rm -r $(SRC_DIR)/opencc/build || true
	rm -r $(SRC_DIR)/yaml-cpp/build || true

glog:
	cd $(SRC_DIR)/glog; \
	cmake . -Bcmake-build \
	-DBUILD_TESTING:BOOL=OFF \
	-DWITH_GFLAGS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(THIRD_PARTY_DIR)" \
	&& cmake --build cmake-build --config Release --target install

leveldb:
	cd $(SRC_DIR)/leveldb; make
	cp -R $(SRC_DIR)/leveldb/include/leveldb $(INCLUDE_DIR)/
	cp $(SRC_DIR)/leveldb/libleveldb.a $(LIB_DIR)/

marisa:
	cd $(SRC_DIR)/marisa-trie; \
	./configure --disable-debug \
	--disable-dependency-tracking \
	--enable-static \
	&& make
	cp -R $(SRC_DIR)/marisa-trie/lib/marisa $(INCLUDE_DIR)/
	cp $(SRC_DIR)/marisa-trie/lib/marisa.h $(INCLUDE_DIR)/
	cp $(SRC_DIR)/marisa-trie/lib/.libs/libmarisa.a $(LIB_DIR)/

opencc:
	cd $(SRC_DIR)/opencc; \
	cmake . -Bbuild \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	&& cmake --build build
	mkdir -p $(INCLUDE_DIR)/opencc
	cp $(SRC_DIR)/opencc/src/*.h* $(INCLUDE_DIR)/opencc/
	cp $(SRC_DIR)/opencc/build/src/libopencc.a $(LIB_DIR)/
	mkdir -p $(DATA_DIR)/opencc
	cp $(SRC_DIR)/opencc/data/config/*.json $(DATA_DIR)/opencc/
	cp $(SRC_DIR)/opencc/build/data/*.ocd $(DATA_DIR)/opencc/

yaml-cpp:
	cd $(SRC_DIR)/yaml-cpp; \
	cmake . -Bbuild \
	-DCMAKE_BUILD_TYPE=Release \
	&& cmake --build build
	cp -R $(SRC_DIR)/yaml-cpp/include/yaml-cpp $(INCLUDE_DIR)/
	cp $(SRC_DIR)/yaml-cpp/build/libyaml-cpp.a $(LIB_DIR)/

gtest:
	cd $(SRC_DIR)/gtest; \
	cmake . -Bbuild \
	&& cmake --build build
	cp -R $(SRC_DIR)/gtest/include/gtest $(INCLUDE_DIR)/
	cp $(SRC_DIR)/gtest/build/libgtest*.a $(LIB_DIR)/
