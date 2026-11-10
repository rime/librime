//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <fstream>
#include <iterator>
#include <rime/algo/syllabifier.h>
#include <rime/dict/table.h>

class RimeTableTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    if (!table_) {
      table_.reset(new rime::Table(rime::path{"table_test.bin"}));
      table_->Remove();
      rime::Syllabary syll;
      rime::Vocabulary voc;
      PrepareSampleVocabulary(syll, voc);
      ASSERT_TRUE(table_->Build(syll, voc, total_num_entries));
      ASSERT_TRUE(table_->Save());
    }
    table_->Load();
  }
  virtual void TearDown() { table_->Close(); }

 protected:
  static const int total_num_entries = 8;

  static void PrepareSampleVocabulary(rime::Syllabary& syll,
                                      rime::Vocabulary& voc);
  static rime::string Text(const rime::TableAccessor& a) {
    return table_->GetEntryText(*a.entry());
  }
  static rime::the<rime::Table> table_;
};

rime::the<rime::Table> RimeTableTest::table_;

#if !defined(_WIN32) || !defined(RIME_IMPORTS)
class InspectableTable : public rime::Table {
 public:
  using Table::Table;
  bool has_builder() const { return bool(string_table_builder_); }
};
#endif

TEST(TableBuilder, CompactSingleSyllableOutputMatchesVocabulary) {
  auto compare_outputs = [](bool sort_by_weight, const char* suffix) {
    const rime::path vocabulary_path{std::string("table_vocabulary_") + suffix +
                                     ".bin"};
    const rime::path compact_path{std::string("table_compact_") + suffix +
                                  ".bin"};
    rime::Table vocabulary_table(vocabulary_path);
    rime::Table compact_table(compact_path);
    vocabulary_table.Remove();
    compact_table.Remove();

    rime::Syllabary syllabary{"a", "b", "c"};
    rime::Vocabulary vocabulary;
    rime::Code a_code;
    a_code.push_back(0);
    rime::Code b_code;
    b_code.push_back(1);
    vocabulary[0].entries.push_back({"first", a_code, 1.0});
    vocabulary[0].entries.push_back({"second", a_code, 2.0});
    vocabulary[1].entries.push_back({"third", b_code, 3.0});

    rime::CompactDictEntryList compact;
    compact.text = "firstsecondthird";
    compact.entries = {{0, 5, 0, 1.0}, {5, 6, 0, 2.0}, {11, 5, 1, 3.0}};
    compact.page_offsets = {0, 2, 3, 3};
    if (sort_by_weight) {
      vocabulary.SortHomophones();
      std::swap(compact.entries[0], compact.entries[1]);
    }

    EXPECT_TRUE(vocabulary_table.Build(syllabary, vocabulary, 3, 17));
    EXPECT_TRUE(compact_table.Build(syllabary, compact, 3, 17));
    EXPECT_TRUE(vocabulary_table.Save());
    EXPECT_TRUE(compact_table.Save());
    vocabulary_table.Close();
    compact_table.Close();

    std::ifstream vocabulary_file(vocabulary_path.c_str(), std::ios::binary);
    std::ifstream compact_file(compact_path.c_str(), std::ios::binary);
    std::vector<char> vocabulary_bytes(
        (std::istreambuf_iterator<char>(vocabulary_file)),
        std::istreambuf_iterator<char>());
    std::vector<char> compact_bytes(
        (std::istreambuf_iterator<char>(compact_file)),
        std::istreambuf_iterator<char>());
    EXPECT_EQ(vocabulary_bytes, compact_bytes);

    vocabulary_table.Remove();
    compact_table.Remove();
  };
  compare_outputs(false, "original");
  compare_outputs(true, "sorted");
}

void RimeTableTest::PrepareSampleVocabulary(rime::Syllabary& syll,
                                            rime::Vocabulary& voc) {
  rime::ShortDictEntry d;
  syll.insert("0");
  // no entries for '0', however
  syll.insert("1");
  d.code.push_back(1);
  d.text = "yi";
  d.weight = 1.0;
  voc[1].entries.push_back(d);
  syll.insert("2");
  d.code.back() = 2;
  d.text = "er";
  voc[2].entries.push_back(d);
  d.text = "liang";
  voc[2].entries.push_back(d);
  d.text = "lia";
  voc[2].entries.push_back(d);
  syll.insert("3");
  d.code.back() = 3;
  d.text = "san";
  voc[3].entries.push_back(d);
  d.text = "sa";
  voc[3].entries.push_back(d);
  syll.insert("4");
  auto lv2 = rime::New<rime::Vocabulary>();
  voc[1].next_level = lv2;
  auto lv3 = rime::New<rime::Vocabulary>();
  (*lv2)[2].next_level = lv3;
  auto lv4 = rime::New<rime::Vocabulary>();
  (*lv3)[3].next_level = lv4;
  d.code.back() = 1;
  d.code.push_back(2);
  d.code.push_back(3);
  d.text = "yi-er-san";
  (*lv3)[3].entries.push_back(d);
  d.code.push_back(4);
  d.text = "yi-er-san-si";
  (*lv4)[-1].entries.push_back(d);
  d.code.resize(3);
  d.code.push_back(2);
  d.code.push_back(1);
  d.text = "yi-er-san-er-yi";
  (*lv4)[-1].entries.push_back(d);
}

TEST_F(RimeTableTest, IntegrityTest) {
  table_.reset(new rime::Table(rime::path{"table_test.bin"}));
  ASSERT_TRUE(bool(table_));
  ASSERT_TRUE(table_->Load());
}

#if !defined(_WIN32) || !defined(RIME_IMPORTS)
TEST_F(RimeTableTest, BuilderReleasedAfterSuccessfulRetry) {
  InspectableTable table(rime::path{"table_retry_test.bin"});
  table.Remove();
  rime::Syllabary syll;
  rime::Vocabulary voc;
  PrepareSampleVocabulary(syll, voc);

  auto tail = (*(*voc[1].next_level)[2].next_level)[3].next_level;
  auto entries = std::move((*tail)[-1].entries);
  tail->erase(-1);
  EXPECT_FALSE(table.Build(syll, voc, total_num_entries));
  EXPECT_TRUE(table.has_builder());

  (*tail)[-1].entries = std::move(entries);
  table.Remove();
  ASSERT_TRUE(table.Build(syll, voc, total_num_entries));
  EXPECT_FALSE(table.has_builder());
  ASSERT_TRUE(table.Save());
  ASSERT_TRUE(table.Load());
  auto result = table.QueryWords(2);
  ASSERT_EQ(3, result.remaining());
  EXPECT_EQ("er", table.GetEntryText(*result.entry()));
  ASSERT_TRUE(result.Next());
  EXPECT_EQ("liang", table.GetEntryText(*result.entry()));
  ASSERT_TRUE(result.Next());
  EXPECT_EQ("lia", table.GetEntryText(*result.entry()));
  table.Remove();
}
#endif

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
