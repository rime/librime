//
// Copyright RIME Developers
// Distributed under the BSD License
//
// custom_phrase.txt is a stabledb table: "phrase <Tab> code [ <Tab> weight ]".
// Entries without a weight column are packed as c=0 d=0 t=0. CreateDictEntry
// must still return them; otherwise table_translator@custom_phrase yields
// nothing (e.g. zdydu → 自定义短语).
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/dict/user_dictionary.h>

using namespace rime;

// Unweighted table/stabledb rows pack as c=0 d=0 t=0. That is "never ticked",
// not a decayed userdb phrase, and must still produce a dict entry.
TEST(UserDictionary, UnweightedTableEntryIsNotDiscarded) {
  auto e = UserDictionary::CreateDictEntry("zdydu \t自定义短语", "c=0 d=0 t=0",
                                           /*present_tick=*/1);
  ASSERT_TRUE(e) << "unweighted custom_phrase row (dee=0, tick=0) was dropped";
  EXPECT_EQ("自定义短语", e->text);
}

// de21e7d4: truly decayed userdb entries stay suppressed.
TEST(UserDictionary, VeryOldDecayedEntryIsDiscarded) {
  auto e = UserDictionary::CreateDictEntry("foo \tbar", "c=1 d=1e-201 t=1000",
                                           /*present_tick=*/1000);
  EXPECT_FALSE(e);
}
