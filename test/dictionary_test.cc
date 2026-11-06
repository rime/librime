//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/encoder.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>

class RimeDictionaryTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    if (!dict_) {
      dict_.reset(new rime::Dictionary(
          "dictionary_test", {},
          {rime::New<rime::Table>(rime::path{"dictionary_test.table.bin"})},
          rime::New<rime::Prism>(rime::path{"dictionary_test.prism.bin"})));
    }
    if (!rebuilt_) {
      dict_->Remove();
      rime::DictCompiler dict_compiler(dict_.get());
      dict_compiler.Compile(rime::path());  // no schema file
      rebuilt_ = true;
    }
    dict_->Load();
  }
  virtual void TearDown() { dict_.reset(); }

 protected:
  static rime::the<rime::Dictionary> dict_;
  static bool rebuilt_;
};

rime::the<rime::Dictionary> RimeDictionaryTest::dict_;
bool RimeDictionaryTest::rebuilt_ = false;

TEST_F(RimeDictionaryTest, Ready) {
  EXPECT_TRUE(dict_->loaded());
}

TEST_F(RimeDictionaryTest, SimpleLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator it;
  dict_->LookupWords(&it, "zhong", false);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("\xe4\xb8\xad", it.Peek()->text);  // 中
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::RawCode raw_code;
  ASSERT_TRUE(dict_->Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("zhong", raw_code.ToString());
}

TEST_F(RimeDictionaryTest, PredictiveLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator it;
  dict_->LookupWords(&it, "z", true);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("\xe5\x92\x8b", it.Peek()->text);  // 咋
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::RawCode raw_code;
  ASSERT_TRUE(dict_->Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("za", raw_code.ToString());
}

TEST_F(RimeDictionaryTest, ScriptLookup) {
  ASSERT_TRUE(dict_->loaded());
  rime::SyllableGraph g;
  rime::Syllabifier s;
  rime::string input("shurufa");
  ASSERT_TRUE(s.BuildSyllableGraph(input, *dict_->prism(), &g) > 0);
  EXPECT_EQ(g.interpreted_length, g.input_length);
  auto c = dict_->Lookup(g, 0);
  ASSERT_TRUE(bool(c));

  ASSERT_TRUE(c->find(3) != c->end());
  rime::DictEntryIterator& d3((*c)[3]);
  EXPECT_FALSE(d3.exhausted());
  auto e1 = d3.Peek();
  ASSERT_TRUE(bool(e1));
  EXPECT_EQ(1, e1->code.size());
  EXPECT_EQ(3, e1->text.length());
  EXPECT_TRUE(d3.Next());

  ASSERT_TRUE(c->find(5) != c->end());
  rime::DictEntryIterator& d5((*c)[5]);
  EXPECT_FALSE(d5.exhausted());
  auto e2 = d5.Peek();
  ASSERT_TRUE(bool(e2));
  EXPECT_EQ(2, e2->code.size());

  ASSERT_TRUE(c->find(7) != c->end());
  rime::DictEntryIterator& d7((*c)[7]);
  EXPECT_FALSE(d7.exhausted());
  auto e3 = d7.Peek();
  ASSERT_TRUE(bool(e3));
  EXPECT_EQ(3, e3->code.size());
  EXPECT_EQ(9, e3->text.length());
  EXPECT_FALSE(d7.Next());
}

TEST_F(RimeDictionaryTest, ScriptLookupWithExclude) {
  ASSERT_TRUE(dict_->loaded());
  rime::SyllableGraph g;
  rime::Syllabifier s;
  rime::string input("gei");
  rime::hash_set<rime::string> blacklist{"\xe7\xbb\x99"};  // 给
  ASSERT_TRUE(s.BuildSyllableGraph(input, *dict_->prism(), &g) > 0);
  EXPECT_EQ(g.interpreted_length, g.input_length);
  auto c = dict_->Lookup(g, 0, &blacklist);
  EXPECT_TRUE(!c);
}

// Entries of a predictive lookup come out best first: shorter remaining code,
// then higher weight. Every entry is visited exactly once.
static void ExpectBestFirst(rime::Dictionary* dict,
                            const rime::string& code,
                            size_t min_entries) {
  rime::DictEntryIterator it;
  dict->LookupWords(&it, code, true);
  const size_t total = it.entry_count();
  ASSERT_GE(total, min_entries);
  size_t n = 0;
  rime::an<rime::DictEntry> prev;
  rime::set<std::pair<rime::string, rime::string>> seen;
  while (!it.exhausted()) {
    auto e = it.Peek();
    ASSERT_TRUE(bool(e));
    rime::RawCode raw_code;
    ASSERT_TRUE(dict->Decode(e->code, &raw_code));
    EXPECT_TRUE(seen.emplace(e->text, raw_code.ToString()).second)
        << "duplicate entry " << e->text;
    // the first entry is taken from the first matching key, unsorted.
    if (n > 1) {
      if (prev->remaining_code_length == e->remaining_code_length) {
        EXPECT_GE(prev->weight, e->weight) << prev->text << " " << e->text;
      } else {
        EXPECT_LT(prev->remaining_code_length, e->remaining_code_length);
      }
    }
    prev = e;
    ++n;
    it.Next();
  }
  EXPECT_EQ(total, n);
}

TEST_F(RimeDictionaryTest, PredictiveLookupOrder) {
  ASSERT_TRUE(dict_->loaded());
  ExpectBestFirst(dict_.get(), "z", 50);
  // an empty code matches every entry in the dictionary
  ExpectBestFirst(dict_.get(), "", 1000);
}

// Iterating a copy must not consume or reorder the original.
TEST_F(RimeDictionaryTest, CopiedIteratorsAreIndependent) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator it;
  dict_->LookupWords(&it, "z", true);
  ASSERT_FALSE(it.exhausted());
  auto drain = [](rime::DictEntryIterator& iter) {
    rime::vector<rime::string> texts;
    while (!iter.exhausted()) {
      texts.push_back(iter.Peek()->text);
      iter.Next();
    }
    return texts;
  };
  rime::DictEntryIterator copy(it);
  rime::DictEntryIterator assigned;
  assigned = it;
  auto from_copy = drain(copy);
  auto from_assigned = drain(assigned);
  auto from_original = drain(it);
  EXPECT_EQ(it.entry_count(), from_original.size());
  EXPECT_EQ(from_original, from_copy);
  EXPECT_EQ(from_original, from_assigned);
}

TEST_F(RimeDictionaryTest, SkipDropsCachedEntry) {
  ASSERT_TRUE(dict_->loaded());
  rime::DictEntryIterator peeked;
  rime::DictEntryIterator fresh;
  dict_->LookupWords(&peeked, "z", true);
  dict_->LookupWords(&fresh, "z", true);
  ASSERT_TRUE(bool(peeked.Peek()));
  ASSERT_TRUE(peeked.Skip(3));
  ASSERT_TRUE(fresh.Skip(3));
  auto a = peeked.Peek();
  auto b = fresh.Peek();
  ASSERT_TRUE(a && b);
  EXPECT_EQ(b->text, a->text);
  EXPECT_EQ(b->code, a->code);
}

TEST_F(RimeDictionaryTest, TableLookupWithExclude) {
  ASSERT_TRUE(dict_->loaded());
  rime::string input("gei");
  rime::hash_set<rime::string> blacklist{"\xe7\xbb\x99"};  // 给
  rime::DictEntryIterator iter;
  size_t keys = dict_->LookupWords(&iter, input, true, 100, &blacklist);
  EXPECT_EQ(keys, 1);
  EXPECT_TRUE(iter.exhausted());
}
