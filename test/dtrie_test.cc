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
  RimeDoubleArrayTrieMapTest() : prism(NULL) {}

  virtual void SetUp() {
    prism = new Prism;
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
    
    prism->Build(keys);
  }

  virtual void TearDown() {
    delete prism;
  }
  
  Prism * prism;
};

TEST_F(RimeDoubleArrayTrieMapTest, save_load) {
  prism->Save("t.dic");
  Prism tmap;
  tmap.Load("t.dic");
  EXPECT_EQ(prism->size(), tmap.size());
}

TEST_F(RimeDoubleArrayTrieMapTest, HasKey) {
  EXPECT_TRUE(prism->HasKey("google"));
  EXPECT_FALSE(prism->HasKey("googlesoft"));
  
  EXPECT_TRUE(prism->HasKey("microsoft"));
  EXPECT_FALSE(prism->HasKey("peoplesoft"));
}

TEST_F(RimeDoubleArrayTrieMapTest, GetValue) {
  int value = -1;
  EXPECT_TRUE(prism->GetValue("adobe", &value));
  EXPECT_EQ(value, 0);
  
  value = -1;
  EXPECT_TRUE(prism->GetValue("baidu", &value));
  EXPECT_EQ(value, 1);
}

TEST_F(RimeDoubleArrayTrieMapTest, CommonPrefixMatch) {
  std::vector<int> result;
  
  prism->CommonPrefixSearch("goodbye", 10, &result);
  //result is good and goodbye.
  EXPECT_EQ(result.size(), 2);
}

TEST_F(RimeDoubleArrayTrieMapTest, ExpandSearch) {
  std::vector<int> result;
  
  prism->ExpandSearch("goo", &result, 10);
  //result is good, google and goodbye.
  std::for_each(result.begin(), result.end(), print);
  EXPECT_EQ(result.size(), 3);
}
