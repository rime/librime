// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-17 Zou xu <zouivex@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/dtrie.h>
#include <string>
#include <vector>
#include <set>

using namespace rime;

void generate_valid_keys(std::size_t num_keys,
    std::set<std::string> *valid_keys) {
  std::vector<char> key;
  while (valid_keys->size() < num_keys) {
    key.resize(1 + (std::rand() % 8));
    for (std::size_t i = 0; i < key.size(); ++i) {
      key[i] = 'A' + (std::rand() % 26);
    }
    valid_keys->insert(std::string(&key[0], key.size()));
  }
}

void generate_invalid_keys(std::size_t num_keys,
    const std::set<std::string> &valid_keys,
    std::set<std::string> *invalid_keys) {
  std::vector<char> key;
  while (invalid_keys->size() < num_keys) {
    key.resize(1 + (std::rand() % 8));
    for (std::size_t i = 0; i < key.size(); ++i) {
      key[i] = 'A' + (std::rand() % 26);
    }
    std::string generated_key(&key[0], key.size());
    if (valid_keys.find(generated_key) == valid_keys.end())
      invalid_keys->insert(std::string(&key[0], key.size()));
  }
}

class RimeDoubleArrayTrieMapTest : public ::testing::Test {
 protected:
  RimeDoubleArrayTrieMapTest() : trie_map(NULL) {}

  virtual void SetUp() {
    trie_map = new TrieMap;
    std::size_t NUM_VALID_KEYS = 1 << 6;
    std::size_t NUM_INVALID_KEYS = 1 << 7;

    std::set<std::string> valid_keys;
    generate_valid_keys(NUM_VALID_KEYS, &valid_keys);

    std::set<std::string> invalid_keys;
    generate_invalid_keys(NUM_INVALID_KEYS, valid_keys, &invalid_keys);
    
    std::vector<int> values(valid_keys.size());
    std::vector<std::string> keys(valid_keys.size());
    int k = 0;
    for(std::set<std::string>::iterator i = valid_keys.begin(); i != valid_keys.end(); ++i, ++k){
      values[k] = std::rand();
      keys[k] = *i;
    }
    
    trie_map->Build(keys, values);
  }

  virtual void TearDown() {
    delete trie_map;
  }
  
  TrieMap * trie_map;
};

TEST_F(RimeDoubleArrayTrieMapTest, save_load) {
  trie_map->Save("t.dic");
  TrieMap tmap;
  tmap.Load("t.dic");
  EXPECT_EQ(trie_map->size(), tmap.size());
}

TEST_F(RimeDoubleArrayTrieMapTest, HasKey) {
  //todo
}

TEST_F(RimeDoubleArrayTrieMapTest, GetValue) {
  //todo
}
