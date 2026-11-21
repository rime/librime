//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dictionary.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/ticket.h>

namespace {

using namespace rime;

class RimePacksSharedPrismTest : public ::testing::Test {
 protected:
  void SetUp() override {
    WritePrimary();
    WritePack();
  }

  void TearDown() override {
    for (const char* name : {"pack_union_test", "pack_union_extra"}) {
      for (const char* suffix : {".dict.yaml", ".table.bin", ".reverse.bin",
                                 ".prism.bin", ".schema.yaml"}) {
        std::remove((string(name) + suffix).c_str());
      }
    }
    std::remove("pack_union_growth.table.bin");
  }

  static string PrismFormat(const path& file) {
    std::ifstream stream(file.c_str(), std::ios::binary);
    char header[prism::Metadata::kFormatMaxLength] = {};
    stream.read(header, sizeof(header));
    return string(header);
  }

  static void WritePrimary() {
    std::ofstream("pack_union_test.dict.yaml")
        << "---\nname: pack_union_test\nversion: '1.0'\nsort: by_weight\n...\n"
        << "甲\tzzz\t100\n"
        << "你\tni\t100\n好\thao\t100\n";
  }

  static void WritePack(const string& weight = "20") {
    std::ofstream("pack_union_extra.dict.yaml")
        << "---\nname: pack_union_extra\nversion: '1.0'\nsort: by_weight\n...\n"
        << "新\taaa\t" << weight << "\n"
        << "异码\taaa bbb ccc ddd\t30\n"
        << "你好\t\t25\n";
  }

  static an<Dictionary> NewDictionary(bool with_pack = true) {
    vector<of<Table>> tables = {New<Table>(path{"pack_union_test.table.bin"})};
    vector<string> packs;
    if (with_pack) {
      tables.push_back(New<Table>(path{"pack_union_extra.table.bin"}));
      packs.push_back("pack_union_extra");
    }
    auto prism = New<Prism>(path{"pack_union_test.prism.bin"});
    return New<Dictionary>("pack_union_test", std::move(packs),
                           std::move(tables), std::move(prism));
  }

  static bool Compile(bool with_pack = true) {
    auto dictionary = NewDictionary(with_pack);
    DictCompiler compiler(dictionary.get());
    return compiler.Compile(path{});
  }

  static set<string> LookupWords(Dictionary* dictionary, const string& input) {
    DictEntryIterator entries;
    dictionary->LookupWords(&entries, input, false);
    set<string> result;
    while (!entries.exhausted()) {
      result.insert(entries.Peek()->text);
      entries.Next();
    }
    return result;
  }

  static set<string> LookupPhrase(Dictionary* dictionary, const string& input) {
    SyllableGraph graph;
    Syllabifier syllabifier;
    EXPECT_EQ(input.size(), syllabifier.BuildSyllableGraph(
                                input, *dictionary->prism(), &graph));
    set<string> result;
    auto collector = dictionary->Lookup(graph, 0);
    if (!collector)
      return result;
    auto iter = collector->find(input.size());
    if (iter == collector->end())
      return result;
    while (!iter->second.exhausted()) {
      result.insert(iter->second.Peek()->text);
      iter->second.Next();
    }
    return result;
  }
};

#if !defined(_WIN32) || !defined(RIME_IMPORTS)
TEST_F(RimePacksSharedPrismTest, TableReacquiresPointersAfterGrowth) {
  // Even a tiny table can need another mmap allocation for MARISA's
  // serialized string table. The resulting remap must not leave metadata_
  // pointing at the old address.
  const path filename{"pack_union_growth.table.bin"};
  Syllabary syllabary{"growth"};
  Vocabulary vocabulary;
  Code code;
  code.push_back(0);
  auto* entries = vocabulary.LocateEntries(code);
  ASSERT_NE(entries, nullptr);
  entries->push_back(ShortDictEntry("测试", code, 1.0));

  {
    Table table(filename);
    ASSERT_TRUE(table.Build(syllabary, vocabulary, 1));
    ASSERT_TRUE(table.Save());
  }

  Table table(filename);
  ASSERT_TRUE(table.Load());
  auto result = table.QueryWords(0);
  ASSERT_FALSE(result.exhausted());
  EXPECT_EQ("测试", table.GetEntryText(*result.entry()));
}

#endif

TEST_F(RimePacksSharedPrismTest, IndependentCodesWorkWithoutRebuildingBase) {
  ASSERT_TRUE(Compile());
  auto dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  ASSERT_TRUE(dictionary->prism()->has_syllabary());
  EXPECT_EQ("Rime::SharedPrism/1.0",
            PrismFormat(path{"pack_union_test.prism.bin"}));
  EXPECT_TRUE(LookupWords(dictionary.get(), "zzz").count("甲"));
  EXPECT_TRUE(LookupWords(dictionary.get(), "aaa").count("新"));
  EXPECT_TRUE(LookupPhrase(dictionary.get(), "aaabbbcccddd").count("异码"));
  EXPECT_TRUE(LookupPhrase(dictionary.get(), "nihao").count("你好"));

  const path primary{"pack_union_test.table.bin"};
  const path reverse{"pack_union_test.reverse.bin"};
  const path pack{"pack_union_extra.table.bin"};
  const path prism{"pack_union_test.prism.bin"};
  const auto marker =
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
  for (const auto& file : {primary, reverse, pack, prism})
    std::filesystem::last_write_time(file, marker);
  const auto primary_time = std::filesystem::last_write_time(primary);
  const auto reverse_time = std::filesystem::last_write_time(reverse);
  const auto pack_time = std::filesystem::last_write_time(pack);
  const auto prism_time = std::filesystem::last_write_time(prism);

  dictionary.reset();
  WritePack("50");
  ASSERT_TRUE(Compile());
  // Weight-only changes rebuild the pack table, but not the base or prism.
  EXPECT_EQ(primary_time, std::filesystem::last_write_time(primary));
  EXPECT_EQ(reverse_time, std::filesystem::last_write_time(reverse));
  EXPECT_NE(pack_time, std::filesystem::last_write_time(pack));
  EXPECT_EQ(prism_time, std::filesystem::last_write_time(prism));
  dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "aaa").count("新"));

  // Backdate again: consecutive rebuilds can share a timestamp on Windows.
  std::filesystem::last_write_time(pack, marker);
  const auto weighted_pack_time = std::filesystem::last_write_time(pack);
  dictionary.reset();
  std::ofstream("pack_union_extra.dict.yaml", std::ios::app) << "独\taab\t90\n";
  ASSERT_TRUE(Compile());
  // New syllables additionally rebuild the shared prism, still not the base.
  EXPECT_EQ(primary_time, std::filesystem::last_write_time(primary));
  EXPECT_EQ(reverse_time, std::filesystem::last_write_time(reverse));
  EXPECT_NE(weighted_pack_time, std::filesystem::last_write_time(pack));
  EXPECT_NE(prism_time, std::filesystem::last_write_time(prism));
  dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "aab").count("独"));
}

TEST_F(RimePacksSharedPrismTest, PackCodeParticipatesInSpellingAlgebra) {
  // Pack-only syllables must be transformed alongside primary syllables.
  // Cover both unmarked umlaut and marked-tone, multi-syllable input.
  std::ofstream("pack_union_extra.dict.yaml", std::ios::app)
      << "哈\tdüe;ab\t10\n"
      << "哈昂\tdǖe;ab dǖe;ab\t10\n";
  std::ofstream("pack_union_test.schema.yaml")
      << "speller:\n  algebra:\n"
      << "    - xform/ǖ([^;]*);/ü$1①;/\n"
      << "    - xlit/ü①/v7/\n"
      << "    - xform/ve([0-9]);/t$1;/\n"
      << "    - derive/^([a-z]+)([0-9]?);.*$/$1$2/\n"
      << "    - derive/^([a-z]+)([0-9]?);.*$/$1/\n"
      << "    - erase/.*;.*/\n";

  auto dictionary = NewDictionary();
  DictCompiler compiler(dictionary.get());
  ASSERT_TRUE(compiler.Compile(path{"pack_union_test.schema.yaml"}));
  dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());

  int spelling_id = -1;
  ASSERT_TRUE(dictionary->prism()->GetValue("dve", &spelling_id));
  EXPECT_TRUE(LookupPhrase(dictionary.get(), "dve").count("哈"));
  ASSERT_TRUE(dictionary->prism()->GetValue("dt7", &spelling_id));
  EXPECT_TRUE(LookupPhrase(dictionary.get(), "dt7dt7").count("哈昂"));
}

TEST_F(RimePacksSharedPrismTest, AddingRemovingPacksKeepsBaseTable) {
  // Starting from a legacy (non-packed) prism must not rewrite the base table.
  ASSERT_TRUE(Compile(false));
  const path primary{"pack_union_test.table.bin"};
  const path reverse{"pack_union_test.reverse.bin"};
  const path prism{"pack_union_test.prism.bin"};
  const auto marker =
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
  std::filesystem::last_write_time(primary, marker);
  std::filesystem::last_write_time(reverse, marker);
  const auto primary_time = std::filesystem::last_write_time(primary);
  const auto reverse_time = std::filesystem::last_write_time(reverse);

  ASSERT_TRUE(Compile(true));
  EXPECT_EQ(primary_time, std::filesystem::last_write_time(primary));
  EXPECT_EQ(reverse_time, std::filesystem::last_write_time(reverse));
  {
    auto dictionary = NewDictionary();
    ASSERT_TRUE(dictionary->Load());
    ASSERT_TRUE(dictionary->prism()->has_syllabary());
    EXPECT_TRUE(LookupWords(dictionary.get(), "aaa").count("新"));
  }

  ASSERT_TRUE(Compile(false));
  EXPECT_EQ(primary_time, std::filesystem::last_write_time(primary));
  EXPECT_EQ(reverse_time, std::filesystem::last_write_time(reverse));
  auto dictionary = NewDictionary(false);
  ASSERT_TRUE(dictionary->Load());
  EXPECT_FALSE(dictionary->prism()->has_syllabary());
  EXPECT_EQ("Rime::Prism/5.0", PrismFormat(path{"pack_union_test.prism.bin"}));
  EXPECT_TRUE(LookupWords(dictionary.get(), "zzz").count("甲"));
}

TEST_F(RimePacksSharedPrismTest, OldExperimentalMagicRebuildsPrismOnly) {
  ASSERT_TRUE(Compile());
  const path primary{"pack_union_test.table.bin"};
  const path pack{"pack_union_extra.table.bin"};
  const path prism{"pack_union_test.prism.bin"};
  const auto marker =
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
  for (const auto& file : {primary, pack})
    std::filesystem::last_write_time(file, marker);
  const auto primary_time = std::filesystem::last_write_time(primary);
  const auto pack_time = std::filesystem::last_write_time(pack);

  // Emulate the old experimental magic without modifying the data.
  {
    std::fstream file(prism.c_str(),
                      std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(file);
    string previous(prism::Metadata::kFormatMaxLength, '\0');
    previous.replace(0, sizeof("Rime::PackedPrism/1.0") - 1,
                     "Rime::PackedPrism/1.0");
    file.write(previous.data(), previous.size());
    ASSERT_TRUE(file);
  }
  {
    Prism legacy(prism);
    EXPECT_FALSE(legacy.Load());
  }
  ASSERT_TRUE(Compile());
  EXPECT_EQ(primary_time, std::filesystem::last_write_time(primary));
  EXPECT_EQ(pack_time, std::filesystem::last_write_time(pack));
  EXPECT_EQ("Rime::SharedPrism/1.0", PrismFormat(prism));
  auto dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "aaa").count("新"));
}

TEST_F(RimePacksSharedPrismTest, NamedPacksLoadThroughTheExistingConfig) {
  ASSERT_TRUE(Compile());
  Schema schema("pack_union_schema", new Config);
  auto config = schema.config();
  ASSERT_TRUE(config->SetString("translator/dictionary", "pack_union_test"));
  auto packs = New<ConfigList>();
  packs->Append(New<ConfigValue>("pack_union_extra"));
  ASSERT_TRUE(schema.config()->SetItem("translator/packs", packs));
  Ticket ticket;
  ticket.schema = &schema;
  ticket.name_space = "translator";
  the<Dictionary> dictionary(Dictionary::Require("dictionary")->Create(ticket));
  ASSERT_TRUE(dictionary);
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "aaa").count("新"));
}

TEST_F(RimePacksSharedPrismTest, MissingPackIsOptionalAtRuntime) {
  ASSERT_TRUE(Compile());
  std::remove("pack_union_extra.table.bin");
  auto dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "zzz").count("甲"));
  EXPECT_FALSE(LookupWords(dictionary.get(), "aaa").count("新"));
}

TEST_F(RimePacksSharedPrismTest, MissingSourceAndTableRemainOptional) {
  ASSERT_TRUE(Compile(false));
  std::remove("pack_union_extra.dict.yaml");
  ASSERT_TRUE(Compile(true));
  auto dictionary = NewDictionary();
  ASSERT_TRUE(dictionary->Load());
  EXPECT_TRUE(LookupWords(dictionary.get(), "zzz").count("甲"));
}

TEST_F(RimePacksSharedPrismTest, NoPacksPreserveTheLegacyPrism) {
  ASSERT_TRUE(Compile(false));
  auto dictionary = NewDictionary(false);
  ASSERT_TRUE(dictionary->Load());
  EXPECT_FALSE(dictionary->prism()->has_syllabary());
  EXPECT_EQ("Rime::Prism/5.0", PrismFormat(path{"pack_union_test.prism.bin"}));
  EXPECT_TRUE(LookupWords(dictionary.get(), "zzz").count("甲"));
}

}  // namespace
