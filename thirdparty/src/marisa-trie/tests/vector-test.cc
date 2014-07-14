#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#include <marisa/grimoire/vector/pop-count.h>
#include <marisa/grimoire/vector/rank-index.h>
#include <marisa/grimoire/vector.h>

#include "marisa-assert.h"

namespace {

#if MARISA_WORD_SIZE == 64
void TestPopCount() {
  TEST_START();

  {
    marisa::grimoire::vector::PopCount count(0);
    ASSERT(count.lo8() == 0);
    ASSERT(count.lo16() == 0);
    ASSERT(count.lo24() == 0);
    ASSERT(count.lo32() == 0);
    ASSERT(count.lo40() == 0);
    ASSERT(count.lo48() == 0);
    ASSERT(count.lo56() == 0);
    ASSERT(count.lo64() == 0);
  }

  {
    marisa::grimoire::vector::PopCount count(0xFFFFFFFFFFFFFFFFULL);
    ASSERT(count.lo8() == 8);
    ASSERT(count.lo16() == 16);
    ASSERT(count.lo24() == 24);
    ASSERT(count.lo32() == 32);
    ASSERT(count.lo40() == 40);
    ASSERT(count.lo48() == 48);
    ASSERT(count.lo56() == 56);
    ASSERT(count.lo64() == 64);
  }

  {
    marisa::grimoire::vector::PopCount count(0xFF7F3F1F0F070301ULL);
    ASSERT(count.lo8() == 1);
    ASSERT(count.lo16() == 3);
    ASSERT(count.lo24() == 6);
    ASSERT(count.lo32() == 10);
    ASSERT(count.lo40() == 15);
    ASSERT(count.lo48() == 21);
    ASSERT(count.lo56() == 28);
    ASSERT(count.lo64() == 36);
  }

  TEST_END();
}
#else  // MARISA_WORD_SIZE == 64
void TestPopCount() {
  TEST_START();

  {
    marisa::grimoire::vector::PopCount count(0);
    ASSERT(count.lo8() == 0);
    ASSERT(count.lo16() == 0);
    ASSERT(count.lo24() == 0);
    ASSERT(count.lo32() == 0);
  }

  {
    marisa::grimoire::vector::PopCount count(0xFFFFFFFFU);
    ASSERT(count.lo8() == 8);
    ASSERT(count.lo16() == 16);
    ASSERT(count.lo24() == 24);
    ASSERT(count.lo32() == 32);
  }

  {
    marisa::grimoire::vector::PopCount count(0xFF3F0F03U);
    ASSERT(count.lo8() == 2);
    ASSERT(count.lo16() == 6);
    ASSERT(count.lo24() == 12);
    ASSERT(count.lo32() == 20);
  }

  TEST_END();
}
#endif  // MARISA_WORD_SIZE == 64

void TestRankIndex() {
  TEST_START();

  marisa::grimoire::vector::RankIndex rank;

  ASSERT(rank.abs() == 0);
  ASSERT(rank.rel1() == 0);
  ASSERT(rank.rel2() == 0);
  ASSERT(rank.rel3() == 0);
  ASSERT(rank.rel4() == 0);
  ASSERT(rank.rel5() == 0);
  ASSERT(rank.rel6() == 0);
  ASSERT(rank.rel7() == 0);

  rank.set_abs(10000);
  rank.set_rel1(64);
  rank.set_rel2(128);
  rank.set_rel3(192);
  rank.set_rel4(256);
  rank.set_rel5(320);
  rank.set_rel6(384);
  rank.set_rel7(448);

  ASSERT(rank.abs() == 10000);
  ASSERT(rank.rel1() == 64);
  ASSERT(rank.rel2() == 128);
  ASSERT(rank.rel3() == 192);
  ASSERT(rank.rel4() == 256);
  ASSERT(rank.rel5() == 320);
  ASSERT(rank.rel6() == 384);
  ASSERT(rank.rel7() == 448);

  TEST_END();
}

void TestVector() {
  TEST_START();

  std::vector<int> values;
  for (std::size_t i = 0; i < 10000; ++i) {
    values.push_back(std::rand());
  }

  marisa::grimoire::Vector<int> vec;

  ASSERT(vec.max_size() == (MARISA_SIZE_MAX / sizeof(int)));
  ASSERT(vec.size() == 0);
  ASSERT(vec.capacity() == 0);
  ASSERT(!vec.fixed());
  ASSERT(vec.empty());
  ASSERT(vec.total_size() == 0);
  ASSERT(vec.io_size() == sizeof(marisa::UInt64));

  for (std::size_t i = 0; i < values.size(); ++i) {
    vec.push_back(values[i]);
    ASSERT(vec[i] == values[i]);
    ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec)[i]
        == values[i]);
  }

  ASSERT(vec.size() == values.size());
  ASSERT(vec.capacity() >= vec.size());
  ASSERT(!vec.empty());
  ASSERT(vec.total_size() == (sizeof(int) * values.size()));
  ASSERT(vec.io_size() == sizeof(marisa::UInt64)
      + ((sizeof(int) * values.size())));

  ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec).front()
      == values.front());
  ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec).back()
      == values.back());
  ASSERT(vec.front() == values.front());
  ASSERT(vec.back() == values.back());

  vec.shrink();

  ASSERT(vec.size() == values.size());
  ASSERT(vec.capacity() == vec.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    ASSERT(vec[i] == values[i]);
    ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec)[i]
        == values[i]);
  }

  {
    marisa::grimoire::Writer writer;
    writer.open("vector-test.dat");
    vec.write(writer);
  }
  vec.clear();

  ASSERT(vec.empty());
  ASSERT(vec.capacity() == 0);

  {
    marisa::grimoire::Mapper mapper;
    mapper.open("vector-test.dat");
    vec.map(mapper);

    ASSERT(vec.size() == values.size());
    ASSERT(vec.capacity() == 0);
    ASSERT(vec.fixed());
    ASSERT(!vec.empty());
    ASSERT(vec.total_size() == (sizeof(int) * values.size()));
    ASSERT(vec.io_size() == sizeof(marisa::UInt64)
        + ((sizeof(int) * values.size())));

    for (std::size_t i = 0; i < values.size(); ++i) {
      ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec)[i]
          == values[i]);
    }

    vec.clear();
  }

  {
    marisa::grimoire::Reader reader;
    reader.open("vector-test.dat");
    vec.read(reader);
  }

  ASSERT(vec.size() == values.size());
  ASSERT(vec.capacity() == vec.size());
  ASSERT(!vec.fixed());
  ASSERT(!vec.empty());
  ASSERT(vec.total_size() == (sizeof(int) * values.size()));
  ASSERT(vec.io_size() == sizeof(marisa::UInt64)
      + ((sizeof(int) * values.size())));

  for (std::size_t i = 0; i < values.size(); ++i) {
    ASSERT(vec[i] == values[i]);
    ASSERT(static_cast<const marisa::grimoire::Vector<int> &>(vec)[i]
        == values[i]);
  }

  vec.clear();

  vec.push_back(0);
  ASSERT(vec.capacity() == 1);
  vec.push_back(1);
  ASSERT(vec.capacity() == 2);
  vec.push_back(2);
  ASSERT(vec.capacity() == 4);
  vec.resize(5);
  ASSERT(vec.capacity() == 8);
  vec.resize(100);
  ASSERT(vec.capacity() == 100);

  EXCEPT(vec.resize(MARISA_SIZE_MAX), MARISA_SIZE_ERROR);

  vec.fix();
  ASSERT(vec.fixed());
  EXCEPT(vec.fix(), MARISA_STATE_ERROR);
  EXCEPT(vec.push_back(0), MARISA_STATE_ERROR);
  EXCEPT(vec.resize(0), MARISA_STATE_ERROR);
  EXCEPT(vec.reserve(0), MARISA_STATE_ERROR);

  TEST_END();
}

void TestFlatVector() {
  TEST_START();

  marisa::grimoire::FlatVector vec;

  ASSERT(vec.value_size() == 0);
  ASSERT(vec.mask() == 0);
  ASSERT(vec.size() == 0);
  ASSERT(vec.empty());
  ASSERT(vec.total_size() == 0);
  ASSERT(vec.io_size() == (sizeof(marisa::UInt64) * 3));

  marisa::grimoire::Vector<marisa::UInt32> values;
  vec.build(values);

  ASSERT(vec.value_size() == 0);
  ASSERT(vec.mask() == 0);
  ASSERT(vec.size() == 0);
  ASSERT(vec.empty());
  ASSERT(vec.total_size() == 0);
  ASSERT(vec.io_size() == (sizeof(marisa::UInt64) * 3));

  values.push_back(0);
  vec.build(values);

  ASSERT(vec.value_size() == 0);
  ASSERT(vec.mask() == 0);
  ASSERT(vec.size() == 1);
  ASSERT(!vec.empty());
  ASSERT(vec.total_size() == 8);
  ASSERT(vec.io_size() == (sizeof(marisa::UInt64) * 4));
  ASSERT(vec[0] == 0);

  values.push_back(255);
  vec.build(values);

  ASSERT(vec.value_size() == 8);
  ASSERT(vec.mask() == 0xFF);
  ASSERT(vec.size() == 2);
  ASSERT(vec[0] == 0);
  ASSERT(vec[1] == 255);

  values.push_back(65536);
  vec.build(values);

  ASSERT(vec.value_size() == 17);
  ASSERT(vec.mask() == 0x1FFFF);
  ASSERT(vec.size() == 3);
  ASSERT(vec[0] == 0);
  ASSERT(vec[1] == 255);
  ASSERT(vec[2] == 65536);

  {
    marisa::grimoire::Writer writer;
    writer.open("vector-test.dat");
    vec.write(writer);
  }

  vec.clear();

  ASSERT(vec.value_size() == 0);
  ASSERT(vec.mask() == 0);
  ASSERT(vec.size() == 0);

  {
    marisa::grimoire::Mapper mapper;
    mapper.open("vector-test.dat");
    vec.map(mapper);

    ASSERT(vec.value_size() == 17);
    ASSERT(vec.mask() == 0x1FFFF);
    ASSERT(vec.size() == 3);
    ASSERT(vec[0] == 0);
    ASSERT(vec[1] == 255);
    ASSERT(vec[2] == 65536);

    vec.clear();
  }

  {
    marisa::grimoire::Reader reader;
    reader.open("vector-test.dat");
    vec.read(reader);
  }

  ASSERT(vec.value_size() == 17);
  ASSERT(vec.mask() == 0x1FFFF);
  ASSERT(vec.size() == 3);
  ASSERT(vec[0] == 0);
  ASSERT(vec[1] == 255);
  ASSERT(vec[2] == 65536);

  values.clear();
  for (std::size_t i = 0; i < 10000; ++i) {
    values.push_back(std::rand());
  }
  vec.build(values);

  ASSERT(vec.size() == values.size());
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT(vec[i] == values[i]);
  }

  TEST_END();
}

void TestBitVector(std::size_t size) {
  marisa::grimoire::BitVector bv;

  ASSERT(bv.size() == 0);
  ASSERT(bv.empty());
  ASSERT(bv.total_size() == 0);
  ASSERT(bv.io_size() == sizeof(marisa::UInt64) * 5);

  std::vector<bool> bits(size);
  std::vector<std::size_t> zeros, ones;
  for (std::size_t i = 0; i < size; ++i) {
    const bool bit = (std::rand() % 2) == 0;
    bits[i] = bit;
    bv.push_back(bit);
    (bit ? ones : zeros).push_back(i);
    ASSERT(bv[i] == bits[i]);
  }

  ASSERT(bv.size() == bits.size());
  ASSERT((size == 0) || !bv.empty());

  bv.build(true, true);

  std::size_t num_zeros = 0, num_ones = 0;
  for (std::size_t i = 0; i < bits.size(); ++i) {
    ASSERT(bv[i] == bits[i]);
    ASSERT(bv.rank0(i) == num_zeros);
    ASSERT(bv.rank1(i) == num_ones);
    ++(bv[i] ? num_ones : num_zeros);
  }
  for (std::size_t i = 0; i < zeros.size(); ++i) {
    ASSERT(bv.select0(i) == zeros[i]);
  }
  for (std::size_t i = 0; i < ones.size(); ++i) {
    ASSERT(bv.select1(i) == ones[i]);
  }
  ASSERT(bv.num_0s() == num_zeros);
  ASSERT(bv.num_1s() == num_ones);

  std::stringstream stream;
  {
    marisa::grimoire::Writer writer;
    writer.open(stream);
    bv.write(writer);
  }

  bv.clear();

  ASSERT(bv.size() == 0);
  ASSERT(bv.empty());
  ASSERT(bv.total_size() == 0);
  ASSERT(bv.io_size() == sizeof(marisa::UInt64) * 5);

  {
    marisa::grimoire::Reader reader;
    reader.open(stream);
    bv.read(reader);
  }

  ASSERT(bv.size() == bits.size());

  num_zeros = 0, num_ones = 0;
  for (std::size_t i = 0; i < bits.size(); ++i) {
    ASSERT(bv[i] == bits[i]);
    ASSERT(bv.rank0(i) == num_zeros);
    ASSERT(bv.rank1(i) == num_ones);
    ++(bv[i] ? num_ones : num_zeros);
  }
  for (std::size_t i = 0; i < zeros.size(); ++i) {
    ASSERT(bv.select0(i) == zeros[i]);
  }
  for (std::size_t i = 0; i < ones.size(); ++i) {
    ASSERT(bv.select1(i) == ones[i]);
  }
  ASSERT(bv.num_0s() == num_zeros);
  ASSERT(bv.num_1s() == num_ones);
}

void TestBitVector() {
  TEST_START();

  TestBitVector(0);
  TestBitVector(1);
  TestBitVector(511);
  TestBitVector(512);
  TestBitVector(513);

  for (int i = 0; i < 100; ++i) {
    TestBitVector(std::rand() % 4096);
  }

  TEST_END();
}

}  // namespace

int main() try {
  std::srand((unsigned int)std::time(NULL));

  TestPopCount();
  TestPopCount();
  TestRankIndex();

  TestVector();
  TestFlatVector();
  TestBitVector();

  return 0;
} catch (const marisa::Exception &ex) {
  std::cerr << ex.what() << std::endl;
  throw;
}
