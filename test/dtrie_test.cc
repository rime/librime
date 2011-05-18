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
#include <map>

using namespace rime;

class RimeDoubleArrayTrieMapTest : public ::testing::Test {
 protected:
  RimeDoubleArrayTrieMapTest() : trie_map(NULL) {}

  virtual void SetUp() {
    trie_map = new TrieMap;
    
    std::map<std::string, int> kvmap;
    kvmap["google"] = 1;
    kvmap["good"] = 2;
    kvmap["microsoft"] = 3;
    kvmap["macrosoft"] = 4;
    kvmap["adobe"] = 5;
    kvmap["yahoo"] = 6;
    kvmap["baidu"] = 7;
    
    //keys should be sorted.
    std::vector<std::string> keys(kvmap.size());
    std::vector<int> values(kvmap.size());
    
    int j = 0;
    for(std::map<std::string, int>::iterator i = kvmap.begin(); i != kvmap.end(); ++i, ++j){
      keys[j] = i->first;
      values[j] = i->second;
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
  EXPECT_TRUE(trie_map->HasKey("google"));
  EXPECT_FALSE(trie_map->HasKey("googlesoft"));
  
  EXPECT_TRUE(trie_map->HasKey("microsoft"));
  EXPECT_FALSE(trie_map->HasKey("peoplesoft"));
}

TEST_F(RimeDoubleArrayTrieMapTest, GetValue) {
  //todo
  int value = -1;
  EXPECT_TRUE(trie_map->GetValue("google", &value));
  EXPECT_EQ(value, 1);
  //std::cout<<value<<std::endl;
  value = -1;
  EXPECT_FALSE(trie_map->GetValue("googlesoft", &value));
  EXPECT_EQ(value, -1);
  
  value = -1;
  EXPECT_TRUE(trie_map->GetValue("adobe", &value));
  EXPECT_EQ(value, 5);
  
  value = -1;
  EXPECT_TRUE(trie_map->GetValue("baidu", &value));
  EXPECT_EQ(value, 7);
}
