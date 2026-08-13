//
// Copyright RIME Developers
// Distributed under the BSD License
//
// custom_phrase.txt is a stabledb table: "phrase <Tab> code [ <Tab> weight ]".
// Entries without a weight column are packed as c=0 d=0 t=0. CreateDictEntry
// must still return them; otherwise table_translator@custom_phrase yields
// nothing (e.g. zdydu → 自定义短语).
//
#include <fstream>
#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/dict/user_db.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/table_translator.h>

using namespace rime;

namespace {

bool HasText(an<Translation> translation, const string& text) {
  if (!translation)
    return false;
  while (!translation->exhausted()) {
    auto cand = translation->Peek();
    if (cand && cand->text() == text)
      return true;
    if (!translation->Next())
      break;
  }
  return false;
}

void WriteCustomPhraseTxt(const path& db_path, const string& body) {
  std::ofstream out(db_path.string());
  ASSERT_TRUE(out.good());
  out << "# Rime table\n";
  out << "# coding: utf-8\n";
  out << "#\n";
  out << "# 码表各字段以制表符（Tab）分隔\n";
  out << "# 编码格式：词条+tab+编码+tab+权重\n";
  out << body;
}

}  // namespace

// Unweighted table/stabledb rows pack as c=0 d=0 t=0. That is "never ticked",
// not a decayed userdb phrase, and must still produce a dict entry.
TEST(UserDictionary, UnweightedTableEntryIsNotDiscarded) {
  UserDbValue v;
  auto e = UserDictionary::CreateDictEntry("zdydu \t自定义短语", v.Pack(),
                                           /*present_tick=*/1);
  ASSERT_TRUE(e) << "unweighted custom_phrase row (dee=0, tick=0) was dropped";
  EXPECT_EQ("自定义短语", e->text);
}

// de21e7d4: truly decayed userdb entries stay suppressed.
TEST(UserDictionary, VeryOldDecayedEntryIsDiscarded) {
  UserDbValue v;
  v.commits = 1;
  v.dee = 1e-201;
  v.tick = 1000;
  auto e = UserDictionary::CreateDictEntry("foo \tbar", v.Pack(),
                                           /*present_tick=*/1000);
  EXPECT_FALSE(e);
}

TEST(TableTranslator, UnweightedCustomPhraseAppears) {
  const string dict_name = "custom_phrase_unweighted_test";
  const path db_path{dict_name + ".txt"};
  WriteCustomPhraseTxt(db_path, "自定义短语\tzdydu\n");

  auto* config = new Config;
  config->SetString("custom_phrase/dictionary", "");
  config->SetString("custom_phrase/user_dict", dict_name);
  config->SetString("custom_phrase/db_class", "stabledb");
  config->SetBool("custom_phrase/enable_completion", false);
  config->SetBool("custom_phrase/enable_sentence", false);
  config->SetDouble("custom_phrase/initial_quality", 99);

  auto* schema = new Schema("custom_phrase_unweighted_test", config);
  the<Engine> engine(Engine::Create());
  ASSERT_TRUE(engine);
  engine->ApplySchema(schema);

  Ticket ticket(engine.get(), "custom_phrase", "table_translator");
  TableTranslator translator(ticket);

  const string input = "zdydu";
  Segment segment(0, input.length());
  segment.tags.insert("abc");
  auto cands = translator.Query(input, segment);

  EXPECT_TRUE(HasText(cands, "自定义短语"))
      << "typing zdydu should yield custom_phrase 自定义短语";
}
