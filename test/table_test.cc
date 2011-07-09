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
  rime::Code c;
  rime::EntryDefinition d;
  syll.insert("0ling");
  // no entries for '0ling', however
  syll.insert("1yi");
  c.push_back(1);
  d.code = c;
  d.text = "1yi";
  d.weight = 1.0;
  voc[c].push_back(d);
  syll.insert("2er");
  syll.insert("2lia");
  syll.insert("2liang");
  c.back() = 2;
  d.code = c;
  d.text = "2er";
  voc[c].push_back(d);
  d.text = "2lia";
  voc[c].push_back(d);
  d.text = "2liang";
  voc[c].push_back(d);
  syll.insert("3sa");
  syll.insert("3san");
  c.back() = 3;
  d.code = c;
  d.text = "3sa";
  voc[c].push_back(d);
  d.text = "3san";
  voc[c].push_back(d);
  
  ASSERT_TRUE(table->Build(syll, voc, 6));
  ASSERT_TRUE(table->Save());
  table.reset();

  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  ASSERT_TRUE(table->Load());

  EXPECT_STREQ("0ling", table->GetSyllable(0));
  EXPECT_STREQ("3san", table->GetSyllable(6));
  
  const rime::TableEntryVector *vec = NULL;
  vec = table->GetEntries(1);
  ASSERT_TRUE(vec);
  ASSERT_EQ(1, vec->size());
  EXPECT_EQ("1yi", (*vec)[0].text);
  EXPECT_EQ(1.0, (*vec)[0].weight);
  
  vec = table->GetEntries(2);
  ASSERT_TRUE(vec);
  ASSERT_EQ(3, vec->size());
  EXPECT_EQ("2er", (*vec)[0].text);
  EXPECT_EQ("2lia", (*vec)[1].text);
  EXPECT_EQ("2liang", (*vec)[2].text);
  
  vec = table->GetEntries(3);
  ASSERT_TRUE(vec);
  ASSERT_EQ(2, vec->size());
  EXPECT_EQ("3sa", (*vec)[0].text);
  EXPECT_EQ("3san", (*vec)[1].text);
}
