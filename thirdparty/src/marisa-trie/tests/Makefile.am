AM_CXXFLAGS = -Wall -Weffc++ -Wextra -D_DEBUG -I${top_srcdir}/lib

TESTS = \
  base-test \
  io-test \
  vector-test \
  trie-test \
  marisa-test

check_PROGRAMS = $(TESTS)

noinst_HEADERS = marisa-assert.h

base_test_SOURCES = base-test.cc
base_test_LDADD = ../lib/libmarisa.la

io_test_SOURCES = io-test.cc
io_test_LDADD = ../lib/libmarisa.la

vector_test_SOURCES = vector-test.cc
vector_test_LDADD = ../lib/libmarisa.la

trie_test_SOURCES = trie-test.cc
trie_test_LDADD = ../lib/libmarisa.la

marisa_test_SOURCES = marisa-test.cc
marisa_test_LDADD = ../lib/libmarisa.la
