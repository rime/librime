// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/dict/syllablizer.h>
#include <rime/dict/dictionary.h>

class RimeDictionaryTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    if (!dict_) {
      dict_.reset(new rime::Dictionary(
          "dictionary_test",
          rime::shared_ptr<rime::Table>(new rime::Table("dictionary_test.table.bin")),
          rime::shared_ptr<rime::Prism>(new rime::Prism("dictionary_test.prism.bin"))));
    }
    if (!rebuilt_) {
      dict_->Remove();
      dict_->Compile("dictionary_test.yaml", "dictionary_test.yaml");
      rebuilt_ = true;
    }
    dict_->Load();
  }
  virtual void TearDown() {
    dict_.reset();
  }
 protected:
  static rime::shared_ptr<rime::Dictionary> dict_;
  static bool rebuilt_;
};

rime::shared_ptr<rime::Dictionary> RimeDictionaryTest::dict_;
bool RimeDictionaryTest::rebuilt_ = false;

TEST_F(RimeDictionaryTest, Ready) {
  EXPECT_TRUE(dict_->loaded());
}

TEST_F(RimeDictionaryTest, SimpleLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator it = dict_->LookupWords("zhong", false);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("\xe4\xb8\xad", it.Peek()->text);  // 中
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::dictionary::RawCode raw_code;
  ASSERT_TRUE(dict_->Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("zhong", raw_code.ToString());
}

TEST_F(RimeDictionaryTest, PredictiveLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator it = dict_->LookupWords("z", true);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("\xe5\x92\x8b", it.Peek()->text);  // 咋
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::dictionary::RawCode raw_code;
  ASSERT_TRUE(dict_->Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("za", raw_code.ToString());
}

TEST_F(RimeDictionaryTest, R10nLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::SyllableGraph g;
  rime::Syllablizer s;
  std::string input("shurufa");
  ASSERT_TRUE(s.BuildSyllableGraph(input, *dict_->prism(), &g) > 0);
  EXPECT_EQ(g.interpreted_length, g.input_length);
  rime::shared_ptr<rime::DictEntryCollector> c(dict_->Lookup(g, 0));
  ASSERT_TRUE(c);

  ASSERT_TRUE(c->find(3) != c->end());
  rime::DictEntryIterator d3((*c)[3]);
  EXPECT_FALSE(d3.exhausted());
  rime::shared_ptr<const rime::DictEntry> e1(d3.Peek());
  ASSERT_TRUE(e1);
  EXPECT_EQ(1, e1->code.size());
  EXPECT_EQ(3, e1->text.length());
  EXPECT_TRUE(d3.Next());

  ASSERT_TRUE(c->find(5) != c->end());
  rime::DictEntryIterator d5((*c)[5]);
  EXPECT_FALSE(d5.exhausted());
  rime::shared_ptr<const rime::DictEntry> e2(d5.Peek());
  ASSERT_TRUE(e2);
  EXPECT_EQ(2, e2->code.size());

  ASSERT_TRUE(c->find(7) != c->end());
  rime::DictEntryIterator d7((*c)[7]);
  EXPECT_FALSE(d7.exhausted());
  rime::shared_ptr<const rime::DictEntry> e3(d7.Peek());
  ASSERT_TRUE(e3);
  EXPECT_EQ(3, e3->code.size());
  EXPECT_EQ(9, e3->text.length());
  EXPECT_FALSE(d7.Next());
}
