#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <marisa.h>

#include "marisa-assert.h"

namespace {

void TestTypes() {
  TEST_START();

  ASSERT(sizeof(marisa_uint8) == 1);
  ASSERT(sizeof(marisa_uint16) == 2);
  ASSERT(sizeof(marisa_uint32) == 4);
  ASSERT(sizeof(marisa_uint64) == 8);

  ASSERT(MARISA_WORD_SIZE == (sizeof(std::size_t) * 8));

  ASSERT(MARISA_UINT8_MAX == 0xFFU);
  ASSERT(MARISA_UINT16_MAX == 0xFFFFU);
  ASSERT(MARISA_UINT32_MAX == 0xFFFFFFFFU);
  ASSERT(MARISA_UINT64_MAX == 0xFFFFFFFFFFFFFFFFULL);

  ASSERT(sizeof(marisa::UInt8) == 1);
  ASSERT(sizeof(marisa::UInt16) == 2);
  ASSERT(sizeof(marisa::UInt32) == 4);
  ASSERT(sizeof(marisa::UInt64) == 8);

  TEST_END();
}

void TestSwap() {
  TEST_START();

  int x = 100, y = 200;
  marisa::swap(x, y);
  ASSERT(x == 200);
  ASSERT(y == 100);

  double a = 1.23, b = 2.34;
  marisa::swap(a, b);
  ASSERT(a == 2.34);
  ASSERT(b == 1.23);

  TEST_END();
}

void TestException() {
  TEST_START();

  try {
    MARISA_THROW(MARISA_OK, "Message");
  } catch (const marisa::Exception &ex) {
    ASSERT(std::strcmp(ex.filename(), __FILE__) == 0);
    ASSERT(ex.line() == (__LINE__ - 3));
    ASSERT(ex.error_code() == MARISA_OK);
    ASSERT(std::strstr(ex.error_message(), "Message") != NULL);
  }

  EXCEPT(MARISA_THROW(MARISA_OK, "OK"), MARISA_OK);
  EXCEPT(MARISA_THROW(MARISA_NULL_ERROR, "NULL"), MARISA_NULL_ERROR);

  TEST_END();
}

void TestKey() {
  TEST_START();

  const char * const str = "apple";

  marisa::Key key;

  ASSERT(key.ptr() == NULL);
  ASSERT(key.length() == 0);

  key.set_str(str);

  ASSERT(key.ptr() == str);
  ASSERT(key.length() == std::strlen(str));

  key.set_str(str, 4);

  ASSERT(key.ptr() == str);
  ASSERT(key.length() == 4);

  key.set_weight(1.0);

  ASSERT(key.weight() == 1.0);

  key.set_id(100);

  ASSERT(key.id() == 100);

  TEST_END();
}

void TestKeyset() {
  TEST_START();

  marisa::Keyset keyset;

  ASSERT(keyset.size() == 0);
  ASSERT(keyset.empty());
  ASSERT(keyset.total_length() == 0);

  std::vector<std::string> keys;
  keys.push_back("apple");
  keys.push_back("orange");
  keys.push_back("banana");

  std::size_t total_length = 0;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    keyset.push_back(keys[i].c_str());
    ASSERT(keyset.size() == (i + 1));
    ASSERT(!keyset.empty());

    total_length += keys[i].length();
    ASSERT(keyset.total_length() == total_length);

    ASSERT(keyset[i].length() == keys[i].length());
    ASSERT(std::memcmp(keyset[i].ptr(), keys[i].c_str(),
        keyset[i].length()) == 0);
    ASSERT(keyset[i].weight() == 1.0);
  }

  keyset.clear();

  marisa::Key key;

  key.set_str("123");
  keyset.push_back(key);
  ASSERT(keyset[0].length() == 3);
  ASSERT(std::memcmp(keyset[0].ptr(), "123", 3) == 0);

  key.set_str("456");
  keyset.push_back(key, '\0');
  ASSERT(keyset[1].length() == 3);
  ASSERT(std::memcmp(keyset[1].ptr(), "456", 3) == 0);
  ASSERT(std::strcmp(keyset[1].ptr(), "456") == 0);

  key.set_str("789");
  keyset.push_back(key, '0');
  ASSERT(keyset[2].length() == 3);
  ASSERT(std::memcmp(keyset[2].ptr(), "789", 3) == 0);
  ASSERT(std::memcmp(keyset[2].ptr(), "7890", 4) == 0);

  ASSERT(keyset.size() == 3);

  keyset.clear();

  ASSERT(keyset.size() == 0);
  ASSERT(keyset.total_length() == 0);

  keys.resize(1000);
  std::vector<float> weights(keys.size());

  total_length = 0;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    keys[i].resize(std::rand() % (marisa::Keyset::EXTRA_BLOCK_SIZE * 2));
    for (std::size_t j = 0; j < keys[i].length(); ++j) {
      keys[i][j] = (char)(std::rand() & 0xFF);
    }
    weights[i] = 100.0F * std::rand() / RAND_MAX;

    keyset.push_back(keys[i].c_str(), keys[i].length(), weights[i]);
    total_length += keys[i].length();
    ASSERT(keyset.total_length() == total_length);
  }

  ASSERT(keyset.size() == keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    ASSERT(keyset[i].length() == keys[i].length());
    ASSERT(std::memcmp(keyset[i].ptr(), keys[i].c_str(),
        keyset[i].length()) == 0);
    ASSERT(keyset[i].weight() == weights[i]);
  }

  keyset.reset();

  ASSERT(keyset.size() == 0);
  ASSERT(keyset.total_length() == 0);

  total_length = 0;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    keys[i].resize(std::rand() % (marisa::Keyset::EXTRA_BLOCK_SIZE * 2));
    for (std::size_t j = 0; j < keys[i].length(); ++j) {
      keys[i][j] = (char)(std::rand() & 0xFF);
    }
    weights[i] = 100.0F * std::rand() / RAND_MAX;

    keyset.push_back(keys[i].c_str(), keys[i].length(), weights[i]);
    total_length += keys[i].length();
    ASSERT(keyset.total_length() == total_length);
  }

  ASSERT(keyset.size() == keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    ASSERT(keyset[i].length() == keys[i].length());
    ASSERT(std::memcmp(keyset[i].ptr(), keys[i].c_str(),
        keyset[i].length()) == 0);
    ASSERT(keyset[i].weight() == weights[i]);
  }

  TEST_END();
}

void TestQuery() {
  TEST_START();

  marisa::Query query;

  ASSERT(query.ptr() == NULL);
  ASSERT(query.length() == 0);
  ASSERT(query.id() == 0);

  const char *str = "apple";
  query.set_str(str);

  ASSERT(query.ptr() == str);
  ASSERT(query.length() == std::strlen(str));

  query.set_str(str, 3);

  ASSERT(query.ptr() == str);
  ASSERT(query.length() == 3);

  query.set_id(100);

  ASSERT(query.id() == 100);

  query.clear();

  ASSERT(query.ptr() == NULL);
  ASSERT(query.length() == 0);
  ASSERT(query.id() == 0);

  TEST_END();
}

void TestAgent() {
  TEST_START();

  marisa::Agent agent;

  ASSERT(agent.query().ptr() == NULL);
  ASSERT(agent.query().length() == 0);
  ASSERT(agent.query().id() == 0);

  ASSERT(agent.key().ptr() == NULL);
  ASSERT(agent.key().length() == 0);

  ASSERT(!agent.has_state());

  const char *query_str = "query";
  const char *key_str = "key";

  agent.set_query(query_str);
  agent.set_query(123);
  agent.set_key(key_str);
  agent.set_key(234);

  ASSERT(agent.query().ptr() == query_str);
  ASSERT(agent.query().length() == std::strlen(query_str));
  ASSERT(agent.query().id() == 123);

  ASSERT(agent.key().ptr() == key_str);
  ASSERT(agent.key().length() == std::strlen(key_str));
  ASSERT(agent.key().id() == 234);

  agent.init_state();

  ASSERT(agent.has_state());

  EXCEPT(agent.init_state(), MARISA_STATE_ERROR);

  agent.clear();

  ASSERT(agent.query().ptr() == NULL);
  ASSERT(agent.query().length() == 0);
  ASSERT(agent.query().id() == 0);

  ASSERT(agent.key().ptr() == NULL);
  ASSERT(agent.key().length() == 0);

  ASSERT(!agent.has_state());

  TEST_END();
}

}  // namespace

int main() try {
  TestTypes();
  TestSwap();
  TestException();
  TestKey();
  TestKeyset();
  TestQuery();
  TestAgent();

  return 0;
} catch (const marisa::Exception &ex) {
  std::cerr << ex.what() << std::endl;
  throw;
}
