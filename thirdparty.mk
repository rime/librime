# a minimal build of third party libraries for static linking

SRC_DIR = thirdparty/src
INCLUDE_DIR = thirdparty/include
LIB_DIR = thirdparty/lib
BIN_DIR = thirdparty/bin
DATA_DIR = thirdparty/data

THIRD_PARTY_LIBS = glog leveldb marisa opencc yaml-cpp gtest

.PHONY: all clean-src $(THIRD_PARTY_LIBS)

all: $(THIRD_PARTY_LIBS)

# note: this won't clean output files under include/, lib/ and bin/.
clean-src:
	rm -r $(SRC_DIR)/glog/build || true
	rm -r $(SRC_DIR)/gtest/build || true
	cd $(SRC_DIR)/leveldb; make clean || true
	cd $(SRC_DIR)/marisa-trie; make distclean || true
	rm -r $(SRC_DIR)/opencc/build || true
	rm -r $(SRC_DIR)/yaml-cpp/build || true

glog:
	cd $(SRC_DIR)/glog; cmake \
	. -Bbuild \
	-DWITH_GFLAGS:BOOL=OFF \
	&& cmake --build build
	cp -R $(SRC_DIR)/glog/build/glog $(INCLUDE_DIR)/
	cp $(SRC_DIR)/glog/src/glog/log_severity.h $(INCLUDE_DIR)/glog/
	cp $(SRC_DIR)/glog/build/libglog.a $(LIB_DIR)/

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
