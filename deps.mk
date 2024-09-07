# a minimal build of third party libraries for static linking

rime_root = $(CURDIR)
src_dir = $(rime_root)/deps

ifndef NOPARALLEL
export MAKEFLAGS+=" -j$$(( $$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 8) + 1)) "
endif

build ?= build
prefix ?= $(rime_root)

rime_deps = glog googletest rocksdb marisa-trie opencc yaml-cpp

.PHONY: all clean clean-dist clean-src $(rime_deps)

all: $(rime_deps)

clean: clean-src clean-dist

clean-dist:
	git rev-parse --is-inside-work-tree > /dev/null && \
	find $(prefix)/bin $(prefix)/include $(prefix)/lib $(prefix)/share \
	-depth -maxdepth 1 \
	-exec bash -c 'git ls-files --error-unmatch "$$0" > /dev/null 2>&1 || rm -rv "$$0"' {} \; || true
	rmdir $(prefix) 2> /dev/null || true

# note: this won't clean output files under bin/, include/, lib/ and share/.
clean-src:
	for dep in $(rime_deps); do \
		rm -r $(src_dir)/$${dep}/$(build) || true; \
	done

glog:
	cd $(src_dir)/glog; \
	cmake . -B$(build) \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	-DBUILD_TESTING:BOOL=OFF \
	-DWITH_GFLAGS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install

googletest:
	cd $(src_dir)/googletest; \
	cmake . -B$(build) \
	-DBUILD_GMOCK:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install

rocksdb:
	cd $(src_dir)/rocksdb; \
	cmake . -B$(build) \
	-DROCKSDB_BUILD_SHARED:BOOL=OFF \
	-DWITH_TESTS:BOOL=OFF \
	-DWITH_BENCHMARK:BOOL=OFF \
	-DWITH_GFLAGS:BOOL=OFF \
	-DWITH_TOOLS:BOOL=OFF \
	-DWITH_CORE_TOOLS:BOOL=OFF \
	-DWITH_BENCHMARK_TOOLS:BOOL=OFF \
	-DFAIL_ON_WARNINGS:BOOL=OFF \
	-DUSE_RTTI:BOOL=ON \
	-DPORTABLE:BOOL=ON \
	-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install

marisa-trie:
	cd $(src_dir)/marisa-trie; \
	cmake . -B$(build) \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install

opencc:
	cd $(src_dir)/opencc; \
	cmake . -B$(build) \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install

yaml-cpp:
	cd $(src_dir)/yaml-cpp; \
	cmake . -B$(build) \
	-DYAML_CPP_BUILD_CONTRIB:BOOL=OFF \
	-DYAML_CPP_BUILD_TESTS:BOOL=OFF \
	-DYAML_CPP_BUILD_TOOLS:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(prefix)" \
	&& cmake --build $(build) --target install
