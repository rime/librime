// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-17 Zou xu <zouivex@gmail.com>
//
#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <rime/dict/prism.h>

using namespace rime;

class RimePrismTest : public ::testing::Test {
 protected:
  RimePrismTest() : prism_(NULL) {}

  virtual void SetUp() {
    prism_ = new Prism("prism_test.bin");
    std::set<std::string> keyset;
    keyset.insert("google");     // 4
    keyset.insert("good");       // 2
    keyset.insert("goodbye");    // 3
    keyset.insert("microsoft");
    keyset.insert("macrosoft");
    keyset.insert("adobe");      // 0 == id
    keyset.insert("yahoo");
    keyset.insert("baidu");      // 1

    prism_->Build(keyset);
  }

  virtual void TearDown() {
    delete prism_;
  }

  Prism *prism_;
};

TEST_F(RimePrismTest, SaveAndLoad) {
  prism_->Remove();
  prism_->Save();

  Prism test(prism_->file_name());
  test.Load();

  EXPECT_EQ(prism_->array_size(), test.array_size());
}

TEST_F(RimePrismTest, HasKey) {
  EXPECT_TRUE(prism_->HasKey("google"));
  EXPECT_FALSE(prism_->HasKey("googlesoft"));

  EXPECT_TRUE(prism_->HasKey("microsoft"));
  EXPECT_FALSE(prism_->HasKey("peoplesoft"));
}

TEST_F(RimePrismTest, GetValue) {
  int value = -1;
  EXPECT_TRUE(prism_->GetValue("adobe", &value));
  EXPECT_EQ(value, 0);

  value = -1;
  EXPECT_TRUE(prism_->GetValue("baidu", &value));
  EXPECT_EQ(value, 1);
}

TEST_F(RimePrismTest, CommonPrefixMatch) {
  std::vector<Prism::Match> result;

  prism_->CommonPrefixSearch("goodbye", &result);
  //result is good and goodbye.
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].value, 2);  // good
  EXPECT_EQ(result[0].length, 4);  // good
  EXPECT_EQ(result[1].value, 3);  // goodbye
  EXPECT_EQ(result[1].length, 7);  // goodbye
}

TEST_F(RimePrismTest, ExpandSearch) {
  std::vector<Prism::Match> result;

  prism_->ExpandSearch("goo", &result, 10);
  //result is good, google and goodbye (ordered by length asc).
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].value, 2);  // good
  EXPECT_EQ(result[0].length, 4);  // good
  EXPECT_EQ(result[1].value, 4);  // google
  EXPECT_EQ(result[1].length, 6);  // google
  EXPECT_EQ(result[2].value, 3);  // goodbye
  EXPECT_EQ(result[2].length, 7);  // goodbye
}
