//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Regression tests for UserDictionary cache path.
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_dictionary.h>
#include <rime/dict/user_db.h>

using namespace rime;

using TestDb = UserDbWrapper<TextDb>;

static an<Dictionary> sys_dict;

static void BuildSysDict() {
  if (sys_dict)
    return;
  sys_dict.reset(new Dictionary("dictionary_test", {},
                                {New<Table>(path{"dictionary_test.table.bin"})},
                                New<Prism>(path{"dictionary_test.prism.bin"})));
  sys_dict->Remove();
  DictCompiler dc(sys_dict.get());
  ASSERT_TRUE(dc.Compile(path()));
  ASSERT_TRUE(sys_dict->Load());
}

class UserDictionaryTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { BuildSysDict(); }

  void SetUp() override {
    db = New<TestDb>(path{"user_dict_test.txt"}, "user_dict_test");
    if (db->Exists())
      db->Remove();
    ASSERT_TRUE(db->Open());
    ASSERT_TRUE(db->MetaUpdate("/tick", "100"));
    ud = std::make_unique<UserDictionary>("user_dict_test", db);
    ud->Attach(sys_dict->primary_table(), sys_dict->prism());
    ASSERT_TRUE(ud->Load());
  }

  void TearDown() override {
    ud.reset();
    db->Close();
    if (db->Exists())
      db->Remove();
  }

  // key format: code + " \t" + text  (trailing space before tab, see
  // user_db.cc)
  void AddEntry(const string& code,
                const string& text,
                int commits = 1,
                TickCount tick = 50) {
    UserDbValue v;
    v.commits = commits;
    v.dee = 0.5;
    v.tick = tick;
    ASSERT_TRUE(db->Update(code + " \t" + text, v.Pack()));
  }

  an<DictEntry> DoLookupWords(const string& input,
                              const string& expected_text,
                              bool predictive = false) {
    UserDictEntryIterator iter;
    ud->LookupWords(&iter, input, predictive, 0, nullptr);
    while (!iter.exhausted()) {
      auto e = iter.Peek();
      if (e && e->text == expected_text)
        return e;
      iter.Next();
    }
    return nullptr;
  }

  an<DictEntry> DoPredictiveLookup(const string& input,
                                   const string& expected_text) {
    SyllableGraph g;
    Syllabifier syllabifier;
    if (syllabifier.BuildSyllableGraph(input, *sys_dict->prism(), &g) <= 0)
      return nullptr;
    int depth = 1;
    auto collector = ud->Lookup(g, 0, 0, depth, 0);
    if (!collector)
      return nullptr;
    for (auto& kv : *collector) {
      auto& iter = kv.second;
      while (!iter.exhausted()) {
        auto e = iter.Peek();
        if (e && e->text == expected_text)
          return e;
        iter.Next();
      }
    }
    return nullptr;
  }

  an<TestDb> db;
  the<UserDictionary> ud;
};

TEST_F(UserDictionaryTest, ExactMatchFields) {
  AddEntry("ni", "你");
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("ni", "你");
  ASSERT_NE(nullptr, e);
  EXPECT_EQ("你", e->text);
  EXPECT_EQ(1, e->commit_count);
}

TEST_F(UserDictionaryTest, ExactMatchMultipleEntries) {
  AddEntry("da", "大");
  AddEntry("da", "打");
  ASSERT_TRUE(ud->Reload());
  auto e1 = DoLookupWords("da", "大");
  ASSERT_NE(nullptr, e1);
  EXPECT_EQ("大", e1->text);
  auto e2 = DoLookupWords("da", "打");
  ASSERT_NE(nullptr, e2);
  EXPECT_EQ("打", e2->text);
}

TEST_F(UserDictionaryTest, UpdateEntryAfterLookup) {
  AddEntry("ni", "你", 1);
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("ni", "你");
  ASSERT_NE(nullptr, e);
  EXPECT_EQ(1, e->commit_count);
  // update via UpdateEntry
  DictEntry update;
  update.text = "你";
  update.custom_code = "ni";
  update.commit_count = 1;
  ASSERT_TRUE(ud->UpdateEntry(update, 1));
  // should reflect the update via pending_
  auto e2 = DoLookupWords("ni", "你");
  ASSERT_NE(nullptr, e2);
  EXPECT_GE(e2->commit_count, 1);
}

TEST_F(UserDictionaryTest, AddNewEntryViaUpdateEntry) {
  DictEntry entry;
  entry.text = "泥";
  entry.custom_code = "ni";
  entry.commit_count = 1;
  ASSERT_TRUE(ud->UpdateEntry(entry, 1));
  auto e = DoLookupWords("ni", "泥");
  ASSERT_NE(nullptr, e);
  EXPECT_EQ("泥", e->text);
}

TEST_F(UserDictionaryTest, DeleteEntry) {
  AddEntry("shi", "是");
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("shi", "是");
  ASSERT_NE(nullptr, e);
  // delete
  DictEntry del;
  del.text = "是";
  del.custom_code = "shi";
  ASSERT_TRUE(ud->UpdateEntry(del, -1));
  // should not appear after delete
  auto e2 = DoLookupWords("shi", "是");
  ASSERT_EQ(nullptr, e2);
}

TEST_F(UserDictionaryTest, DeleteThenRevive) {
  AddEntry("bu", "不");
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("bu", "不");
  ASSERT_NE(nullptr, e);
  DictEntry del;
  del.text = "不";
  del.custom_code = "bu";
  ASSERT_TRUE(ud->UpdateEntry(del, -1));
  // revive
  DictEntry revive;
  revive.text = "不";
  revive.custom_code = "bu";
  ASSERT_TRUE(ud->UpdateEntry(revive, 1));
  auto e2 = DoLookupWords("bu", "不");
  ASSERT_NE(nullptr, e2);
  EXPECT_EQ("不", e2->text);
}

TEST_F(UserDictionaryTest, PredictiveLookup) {
  AddEntry("ni", "你");
  AddEntry("ni hao", "你好");
  ASSERT_TRUE(ud->Reload());
  auto e = DoPredictiveLookup("ni", "你好");
  ASSERT_NE(nullptr, e) << "predictive lookup should find '你好' via 'ni'";
  EXPECT_EQ("你好", e->text);
  EXPECT_GT(e->matching_code_size, 0);
}

// TransactionAbort is not applicable to TextDb (no transaction support).
// Tests using LevelDb-backed user dicts should cover this.

TEST_F(UserDictionaryTest, BatchAddThenLookup) {
  const int kCount = 50;
  for (int i = 0; i < kCount; ++i) {
    DictEntry e;
    e.text = "test" + std::to_string(i);
    e.custom_code = "ni";
    e.commit_count = 1;
    ASSERT_TRUE(ud->UpdateEntry(e, 1));
  }
  UserDictEntryIterator iter;
  ud->LookupWords(&iter, "ni", true, 0, nullptr);
  ASSERT_GT(iter.cache_size(), 0);
}

TEST_F(UserDictionaryTest, MultipleCodeSyllables) {
  AddEntry("ni hao", "你好");
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("ni hao", "你好");
  ASSERT_NE(nullptr, e);
  EXPECT_EQ("你好", e->text);
}

TEST_F(UserDictionaryTest, UpdateEntryRoundTrip) {
  AddEntry("ren", "人");
  ASSERT_TRUE(ud->Reload());
  auto e = DoLookupWords("ren", "人");
  ASSERT_NE(nullptr, e);
  DictEntry update;
  update.text = e->text;
  update.code = e->code;
  update.custom_code = "ren";
  update.commit_count = e->commit_count;
  ASSERT_TRUE(ud->UpdateEntry(update, 1));
}
