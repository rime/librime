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
#include <algorithm>

using namespace rime;

void print(int i){
  std::cout<<i<<std::endl;
}

class RimeDoubleArrayTrieMapTest : public ::testing::Test {
 protected:
  RimeDoubleArrayTrieMapTest() : trie_map(NULL) {}

  virtual void SetUp() {
    trie_map = new TrieMap;
    std::set<std::string> keyset;
    keyset.insert("google");
    keyset.insert("good");
    keyset.insert("goodbye");
    keyset.insert("microsoft");
    keyset.insert("macrosoft");
    keyset.insert("adobe");
    keyset.insert("yahoo");
    keyset.insert("baidu");
    
    //keys should be sorted.
    std::vector<std::string> keys(keyset.size());
    
    int j = 0;
    for(std::set<std::string>::iterator i = keyset.begin(); i != keyset.end(); ++i, ++j){
      keys[j] = i->c_str();
    }
    
    trie_map->Build(keys);
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
  EXPECT_TRUE(trie_map->HasKey("google"));
  EXPECT_FALSE(trie_map->HasKey("googlesoft"));
  
  EXPECT_TRUE(trie_map->HasKey("microsoft"));
  EXPECT_FALSE(trie_map->HasKey("peoplesoft"));
}

TEST_F(RimeDoubleArrayTrieMapTest, GetValue) {
  int value = -1;
  EXPECT_TRUE(trie_map->GetValue("adobe", &value));
  EXPECT_EQ(value, 0);
  
  value = -1;
  EXPECT_TRUE(trie_map->GetValue("baidu", &value));
  EXPECT_EQ(value, 1);
}

TEST_F(RimeDoubleArrayTrieMapTest, CommonPrefixMatch) {
  std::vector<int> result;
  
  trie_map->CommonPrefixSearch("goodbye", 10, &result);
  //result is good and goodbye
  EXPECT_EQ(result.size(), 2);
}

TEST_F(RimeDoubleArrayTrieMapTest, ExpandSearch) {
  std::vector<int> result;
  
  trie_map->ExpandSearch("goo", &result, 10);
  //result is good and goodbye
  std::for_each(result.begin(), result.end(), print);
  EXPECT_EQ(result.size(), 3);
}
