//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/table.h>


class RimeTableTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    if (!table_) {
      table_.reset(new rime::Table(file_name));
      table_->Remove();
      rime::Syllabary syll;
      rime::Vocabulary voc;
      PrepareSampleVocabulary(syll, voc);
      ASSERT_TRUE(table_->Build(syll, voc, total_num_entries));
      ASSERT_TRUE(table_->Save());
    }
    table_->Load();
  }
  virtual void TearDown() {
    table_->Close();
  }
 protected:
  static const int total_num_entries = 8;
  static const char file_name[];

  static void PrepareSampleVocabulary(rime::Syllabary& syll,
                                      rime::Vocabulary& voc);
  static rime::string Text(const rime::TableAccessor& a) {
    return table_->GetEntryText(*a.entry());
  }
  static rime::the<rime::Table> table_;
};

const char RimeTableTest::file_name[] = "table_test.bin";

rime::the<rime::Table> RimeTableTest::table_;

void RimeTableTest::PrepareSampleVocabulary(rime::Syllabary& syll,
                                            rime::Vocabulary& voc) {
  auto d = rime::New<rime::DictEntry>();
  syll.insert("0");
  // no entries for '0', however
  syll.insert("1");
  d->code.push_back(1);
  d->text = "yi";
  d->weight = 1.0;
  voc[1].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  syll.insert("2");
  d->code.back() = 2;
  d->text = "er";
  voc[2].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  d->text = "liang";
  voc[2].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  d->text = "lia";
  voc[2].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  syll.insert("3");
  d->code.back() = 3;
  d->text = "san";
  voc[3].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  d->text = "sa";
  voc[3].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  syll.insert("4");
  auto lv2 = rime::New<rime::Vocabulary>();
  voc[1].next_level = lv2;
  auto lv3 = rime::New<rime::Vocabulary>();
  (*lv2)[2].next_level = lv3;
  auto lv4 = rime::New<rime::Vocabulary>();
  (*lv3)[3].next_level = lv4;
  d->code.back() = 1;
  d->code.push_back(2);
  d->code.push_back(3);
  d->text = "yi-er-san";
  (*lv3)[3].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  d->code.push_back(4);
  d->text = "yi-er-san-si";
  (*lv4)[-1].entries.push_back(d);
  d = rime::New<rime::DictEntry>(*d);
  d->code.resize(3);
  d->code.push_back(2);
  d->code.push_back(1);
  d->text  = "yi-er-san-er-yi";
  (*lv4)[-1].entries.push_back(d);
}

TEST_F(RimeTableTest, IntegrityTest) {
  table_.reset(new rime::Table(file_name));
  ASSERT_TRUE(bool(table_));
  ASSERT_TRUE(table_->Load());
}

TEST_F(RimeTableTest, SimpleQuery) {
  EXPECT_STREQ("0", table_->GetSyllableById(0).c_str());
  EXPECT_STREQ("3", table_->GetSyllableById(3).c_str());
  EXPECT_STREQ("4", table_->GetSyllableById(4).c_str());

  rime::TableAccessor v = table_->QueryWords(1);
  ASSERT_FALSE(v.exhausted());
  ASSERT_EQ(1, v.remaining());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi", Text(v).c_str());
  EXPECT_EQ(1.0, v.entry()->weight);
  EXPECT_FALSE(v.Next());

  v = table_->QueryWords(2);
  ASSERT_EQ(3, v.remaining());
  EXPECT_STREQ("er", Text(v).c_str());
  v.Next();
  EXPECT_STREQ("liang", Text(v).c_str());
  v.Next();
  EXPECT_STREQ("lia", Text(v).c_str());

  v = table_->QueryWords(3);
  ASSERT_EQ(2, v.remaining());
  EXPECT_STREQ("san", Text(v).c_str());
  v.Next();
  EXPECT_STREQ("sa", Text(v).c_str());

  rime::Code code;
  code.push_back(1);
  code.push_back(2);
  code.push_back(3);
  v = table_->QueryPhrases(code);
  ASSERT_FALSE(v.exhausted());
  ASSERT_EQ(1, v.remaining());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi-er-san", Text(v).c_str());
  ASSERT_TRUE(v.extra_code() == NULL);
  EXPECT_FALSE(v.Next());

  code.push_back(4);
  v = table_->QueryPhrases(code);
  EXPECT_FALSE(v.exhausted());
  EXPECT_EQ(2, v.remaining());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi-er-san-si", Text(v).c_str());
  ASSERT_TRUE(v.extra_code() != NULL);
  ASSERT_EQ(1, v.extra_code()->size);
  EXPECT_EQ(4, *v.extra_code()->at);
  EXPECT_TRUE(v.Next());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi-er-san-er-yi", Text(v).c_str());
  ASSERT_TRUE(v.extra_code() != NULL);
  ASSERT_EQ(2, v.extra_code()->size);
  EXPECT_EQ(2, v.extra_code()->at[0]);
  EXPECT_EQ(1, v.extra_code()->at[1]);
}

TEST_F(RimeTableTest, QueryWithSyllableGraph) {
  const rime::string input("yiersansi");
  rime::SyllableGraph g;
  g.input_length = input.length();
  g.interpreted_length = g.input_length;
  g.vertices[0] = rime::kNormalSpelling;
  g.vertices[2] = rime::kNormalSpelling;
  g.vertices[4] = rime::kNormalSpelling;
  g.vertices[7] = rime::kNormalSpelling;
  g.vertices[9] = rime::kNormalSpelling;
  g.edges[0][2][1].type = rime::kNormalSpelling;
  g.edges[0][2][1].end_pos = 2;
  g.edges[2][4][2].type = rime::kNormalSpelling;
  g.edges[2][4][2].end_pos = 4;
  g.edges[4][7][3].type = rime::kNormalSpelling;
  g.edges[4][7][3].end_pos = 7;
  g.edges[7][9][4].type = rime::kNormalSpelling;
  g.edges[7][9][4].end_pos = 9;
  g.indices[0][1].push_back(&g.edges[0][2][1]);
  g.indices[2][2].push_back(&g.edges[2][4][2]);
  g.indices[4][3].push_back(&g.edges[4][7][3]);
  g.indices[7][4].push_back(&g.edges[7][9][4]);

  rime::TableQueryResult result;
  ASSERT_TRUE(table_->Query(g, 0, &result));
  EXPECT_EQ(2, result.size());
  ASSERT_TRUE(result.find(2) != result.end());
  ASSERT_EQ(1, result[2].size());
  EXPECT_STREQ("yi", Text(result[2].front()).c_str());
  ASSERT_TRUE(result.find(7) != result.end());
  ASSERT_EQ(2, result[7].size());
  EXPECT_STREQ("yi-er-san", Text(result[7].front()).c_str());
  EXPECT_STREQ("yi-er-san-si", Text(result[7].back()).c_str());
  ASSERT_EQ(1, result[7].back().extra_code()->size);
  EXPECT_EQ(4, result[7].back().extra_code()->at[0]);
  ASSERT_TRUE(result.find(6) == result.end());
  ASSERT_TRUE(result.find(7) != result.end());

  ASSERT_TRUE(table_->Query(g, 2, &result));
  EXPECT_EQ(1, result.size());
  ASSERT_TRUE(result.find(4) != result.end());
  ASSERT_EQ(1, result[4].size());
  EXPECT_STREQ("er", Text(result[4].front()).c_str());
  EXPECT_TRUE(result[4].front().Next());
  EXPECT_STREQ("liang", Text(result[4].front()).c_str());
  EXPECT_TRUE(result[4].front().Next());
  EXPECT_STREQ("lia", Text(result[4].front()).c_str());
  EXPECT_FALSE(result[4].front().Next());
}
