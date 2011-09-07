// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/impl/syllablizer.h>
#include <rime/impl/table.h>

static const char file_name[] = "table_test.bin";

static void prepare_sample_vocabulary(rime::Syllabary &syll,
                                      rime::Vocabulary &voc) {
  rime::DictEntry d;
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
  rime::Vocabulary *lv2 = new rime::Vocabulary;
  voc[1].next_level.reset(lv2);
  rime::Vocabulary *lv3 = new rime::Vocabulary;
  (*lv2)[2].next_level.reset(lv3);
  rime::Vocabulary *lv4 = new rime::Vocabulary;
  (*lv3)[3].next_level.reset(lv4);
  d.code.back() = 1;
  d.code.push_back(2);
  d.code.push_back(3);
  d.code.push_back(4);
  d.text = "yi-er-san-si";
  (*lv4)[-1].entries.push_back(d);
}  

TEST(RimeTableTest, IntegrityTest) {
  rime::scoped_ptr<rime::Table> table;
  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  table->Remove();

  rime::Syllabary syll;
  rime::Vocabulary voc;
  prepare_sample_vocabulary(syll, voc);
  ASSERT_TRUE(table->Build(syll, voc, 7));
  ASSERT_TRUE(table->Save());
  table.reset();

  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  ASSERT_TRUE(table->Load());

}

TEST(RimeTableTest, SimpleQuery) {
  rime::scoped_ptr<rime::Table> table;
  table.reset(new rime::Table(file_name));
  if (!table->Load()) {
    rime::Syllabary syll;
    rime::Vocabulary voc;
    prepare_sample_vocabulary(syll, voc);
    ASSERT_TRUE(table->Build(syll, voc, 7));
    ASSERT_TRUE(table->Save());
  }
  
  EXPECT_STREQ("0", table->GetSyllableById(0));
  EXPECT_STREQ("3", table->GetSyllableById(3));
  EXPECT_STREQ("4", table->GetSyllableById(4));
  
  rime::TableAccessor v = table->QueryWords(1);
  ASSERT_FALSE(v.exhausted());
  ASSERT_EQ(1, v.remaining());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi", v.entry()->text.c_str());
  EXPECT_EQ(1.0, v.entry()->weight);
  EXPECT_FALSE(v.Next());
  
  v = table->QueryWords(2);
  ASSERT_EQ(3, v.remaining());
  EXPECT_STREQ("er", v.entry()->text.c_str());
  v.Next();
  EXPECT_STREQ("liang", v.entry()->text.c_str());
  v.Next();
  EXPECT_STREQ("lia", v.entry()->text.c_str());
  
  v = table->QueryWords(3);
  ASSERT_EQ(2, v.remaining());
  EXPECT_STREQ("san", v.entry()->text.c_str());
  v.Next();
  EXPECT_STREQ("sa", v.entry()->text.c_str());

  rime::Code code;
  code.push_back(1);
  code.push_back(2);
  code.push_back(3);
  code.push_back(4);
  v = table->QueryPhrases(code);
  ASSERT_FALSE(v.exhausted());
  ASSERT_EQ(1, v.remaining());
  ASSERT_TRUE(v.entry() != NULL);
  EXPECT_STREQ("yi-er-san-si", v.entry()->text.c_str());
  ASSERT_TRUE(v.extra_code() != NULL);
  ASSERT_EQ(1, v.extra_code()->size);
  EXPECT_EQ(4, *v.extra_code()->at);
  EXPECT_FALSE(v.Next());
}

TEST(RimeTableTest, QueryWithSyllableGraph) {
  rime::scoped_ptr<rime::Table> table;
  table.reset(new rime::Table(file_name));
  if (!table->Load()) {
    rime::Syllabary syll;
    rime::Vocabulary voc;
    prepare_sample_vocabulary(syll, voc);
    ASSERT_TRUE(table->Build(syll, voc, 7));
    ASSERT_TRUE(table->Save());
  }

  const std::string input("yiersansi");
  rime::SyllableGraph g;
  g.input_length = input.length();
  g.interpreted_length = g.input_length;
  g.vertices[0] = rime::kNormalSpelling;
  g.vertices[2] = rime::kNormalSpelling;
  g.vertices[4] = rime::kNormalSpelling;
  g.vertices[7] = rime::kNormalSpelling;
  g.vertices[9] = rime::kNormalSpelling;
  g.edges[0][2][1].type = rime::kNormalSpelling;
  g.edges[2][4][2].type = rime::kNormalSpelling;
  g.edges[4][7][3].type = rime::kNormalSpelling;
  g.edges[7][9][4].type = rime::kNormalSpelling;
  rime::TableQueryResult result;
  ASSERT_TRUE(table->Query(g, 0, &result));
  EXPECT_EQ(2, result.size());
  ASSERT_TRUE(result.find(2) != result.end());
  ASSERT_EQ(1, result[2].size());
  EXPECT_STREQ("yi", result[2].front().entry()->text.c_str());
  ASSERT_TRUE(result.find(7) != result.end());
  ASSERT_EQ(1, result[7].size());
  EXPECT_STREQ("yi-er-san-si", result[7].front().entry()->text.c_str());
  EXPECT_EQ(1, result[7].front().extra_code()->size);
  EXPECT_EQ(4, result[7].front().extra_code()->at[0]);
  ASSERT_TRUE(result.find(6) == result.end());
  ASSERT_TRUE(result.find(7) != result.end());
}
