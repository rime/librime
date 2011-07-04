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

  rime::Vocabulary voc;
  rime::Code c;
  rime::EntryDefinition d;
  c.push_back(1);
  d.code = c;
  d.text = "yi";
  d.weight = 1.0;
  voc[c].push_back(d);
  c.back() = 2;
  d.code = c;
  d.text = "er";
  voc[c].push_back(d);
  d.text = "liang";
  voc[c].push_back(d);
  d.text = "lia";
  voc[c].push_back(d);
  c.back() = 3;
  d.code = c;
  d.text = "san";
  voc[c].push_back(d);
  d.text = "sa";
  voc[c].push_back(d);
  
  ASSERT_TRUE(table->Build(voc, 4, 6));
  ASSERT_TRUE(table->Save());
  table.reset();

  table.reset(new rime::Table(file_name));
  ASSERT_TRUE(table);
  ASSERT_TRUE(table->Load());

  const rime::TableEntryVector *vec = NULL;
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
