// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/impl/table.h>

TEST(RimeTableTest, Lv1) {
  const char file_name[] = "table_test.bin";
  rime::scoped_ptr<rime::Table> table;
  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  table->Remove();

  rime::Syllabary syll;
  rime::Vocabulary voc;
  rime::DictEntry d;
  syll.insert("0");
  // no entries for '0', however
  syll.insert("1");
  d.code.push_back(1);
  d.text = "yi";
  d.weight = 1.0;
  voc[d.code].push_back(d);
  syll.insert("2");
  d.code.back() = 2;
  d.text = "er";
  voc[d.code].push_back(d);
  d.text = "liang";
  voc[d.code].push_back(d);
  d.text = "lia";
  voc[d.code].push_back(d);
  syll.insert("3");
  d.code.back() = 3;
  d.text = "san";
  voc[d.code].push_back(d);
  d.text = "sa";
  voc[d.code].push_back(d);
  
  ASSERT_TRUE(table->Build(syll, voc, 6));
  ASSERT_TRUE(table->Save());
  table.reset();

  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  ASSERT_TRUE(table->Load());

  EXPECT_STREQ("0", table->GetSyllableById(0));
  EXPECT_STREQ("3", table->GetSyllableById(3));
  
  const rime::table::EntryVector *vec = NULL;
  vec = table->GetEntries(1);
  ASSERT_TRUE(vec);
  ASSERT_EQ(1, vec->size());
  EXPECT_EQ("yi", (*vec)[0].text);
  EXPECT_EQ(1.0, (*vec)[0].weight);
  
  vec = table->GetEntries(2);
  ASSERT_TRUE(vec);
  ASSERT_EQ(3, vec->size());
  EXPECT_EQ("er", (*vec)[0].text);
  EXPECT_EQ("liang", (*vec)[1].text);
  EXPECT_EQ("lia", (*vec)[2].text);
  
  vec = table->GetEntries(3);
  ASSERT_TRUE(vec);
  ASSERT_EQ(2, vec->size());
  EXPECT_EQ("san", (*vec)[0].text);
  EXPECT_EQ("sa", (*vec)[1].text);
}
