AM_CXXFLAGS = -Wall -Weffc++ -Wextra
#AM_CXXFLAGS = -Wall -Weffc++ -Wextra -D_DEBUG

lib_LTLIBRARIES = libmarisa.la

libmarisa_la_SOURCES = \
  marisa/keyset.cc \
  marisa/agent.cc \
  marisa/trie.cc \
  marisa/grimoire/io/mapper.cc \
  marisa/grimoire/io/reader.cc \
  marisa/grimoire/io/writer.cc \
  marisa/grimoire/vector/bit-vector.cc \
  marisa/grimoire/trie/tail.cc \
  marisa/grimoire/trie/louds-trie.cc

include_HEADERS = marisa.h

libmarisa_includedir = ${includedir}/marisa
libmarisa_include_HEADERS = \
  marisa/base.h \
  marisa/exception.h \
  marisa/scoped-ptr.h \
  marisa/scoped-array.h \
  marisa/key.h \
  marisa/keyset.h \
  marisa/query.h \
  marisa/agent.h \
  marisa/stdio.h \
  marisa/iostream.h \
  marisa/trie.h

noinst_HEADERS = \
  marisa/grimoire/intrin.h \
  marisa/grimoire/io.h \
  marisa/grimoire/io/mapper.h \
  marisa/grimoire/io/reader.h \
  marisa/grimoire/io/writer.h \
  marisa/grimoire/vector.h \
  marisa/grimoire/vector/pop-count.h \
  marisa/grimoire/vector/rank-index.h \
  marisa/grimoire/vector/vector.h \
  marisa/grimoire/vector/flat-vector.h \
  marisa/grimoire/vector/bit-vector.h \
  marisa/grimoire/algorithm.h \
  marisa/grimoire/algorithm/sort.h \
  marisa/grimoire/trie.h \
  marisa/grimoire/trie/config.h \
  marisa/grimoire/trie/header.h \
  marisa/grimoire/trie/key.h \
  marisa/grimoire/trie/range.h \
  marisa/grimoire/trie/entry.h \
  marisa/grimoire/trie/tail.h \
  marisa/grimoire/trie/cache.h \
  marisa/grimoire/trie/history.h \
  marisa/grimoire/trie/state.h \
  marisa/grimoire/trie/louds-trie.h
