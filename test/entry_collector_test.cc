//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <filesystem>
#include <fstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <gtest/gtest.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/entry_collector.h>

#if !defined(_WIN32) || !defined(RIME_IMPORTS)
namespace {

class TestEntryCollector : public rime::EntryCollector {
 public:
  using EntryCollector::Collect;
  using EntryCollector::Finish;

  size_t collection_size() const { return collection.size(); }
  bool has_preset() const { return bool(preset_vocabulary); }
};

class RimeEntryCollectorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = std::filesystem::temp_directory_path() /
           boost::uuids::to_string(boost::uuids::random_generator()());
    created_ = std::filesystem::create_directory(dir_);
    ASSERT_TRUE(created_);
  }

  void TearDown() override {
    if (created_)
      std::filesystem::remove_all(dir_);
  }

  rime::path WriteFile(const char* name, const char* contents) {
    rime::path file(dir_ / name);
    std::ofstream out(file.c_str());
    out << contents;
    EXPECT_TRUE(out.good());
    return file;
  }

  void ExpectEntry(const TestEntryCollector& collector,
                   const char* text,
                   const char* code,
                   double weight) {
    size_t matches = 0;
    for (const auto& entry : collector.entries) {
      if (entry.text == text) {
        ++matches;
        EXPECT_EQ(code, entry.raw_code.ToString());
        EXPECT_DOUBLE_EQ(weight, entry.weight);
      }
    }
    EXPECT_EQ(1u, matches) << text;
  }

  std::filesystem::path dir_;
  bool created_ = false;
};

TEST_F(RimeEntryCollectorTest, WithoutPresetDoesNotCollectWords) {
  auto file = WriteFile("first.dict.yaml",
                        "---\nname: first\nversion: '1'\n...\n"
                        "a\ta\t10\n"
                        "b\tb\t20\n"
                        "ab\t\t30\n");
  rime::DictSettings settings;
  TestEntryCollector collector;
  collector.Configure(&settings);
  collector.Collect(file);
  EXPECT_EQ(0u, collector.collection_size());
  collector.Finish();
  EXPECT_FALSE(collector.has_preset());
  EXPECT_EQ(3u, collector.num_entries);
  ExpectEntry(collector, "a", "a", 10);
  ExpectEntry(collector, "b", "b", 20);
  ExpectEntry(collector, "ab", "a b", 30);
}

TEST_F(RimeEntryCollectorTest, MultipleWordCodesAreSortedAndDeduplicated) {
  TestEntryCollector collector;
  collector.CreateEntry("word", "z", "1");
  collector.CreateEntry("word", "a", "1");
  collector.CreateEntry("word", "a", "2");

  std::vector<rime::string> codes;
  ASSERT_TRUE(collector.TranslateWord("word", &codes));
  ASSERT_EQ(2u, codes.size());
  EXPECT_EQ("a", codes[0]);
  EXPECT_EQ("z", codes[1]);
  EXPECT_EQ(2u, collector.num_entries);
}

TEST_F(RimeEntryCollectorTest, PresetWeightsDeduplicationAndRelease) {
  auto preset = WriteFile("preset.txt",
                          "a\t100\n"
                          "b\t80\n"
                          "ab\t60\n"
                          "ba\t40\n");
  auto first = WriteFile("first.dict.yaml",
                         "---\nname: first\nversion: '1'\n...\n"
                         "a\ta\n"
                         "b\tb\t50%\n");
  auto second = WriteFile("second.dict.yaml",
                          "---\nname: second\nversion: '1'\n...\n"
                          "ab\t\t50%\n");
  rime::DictSettings settings;
  preset.replace_extension();
  settings["vocabulary"] = preset.to_utf8_string();
  TestEntryCollector collector;
  collector.Configure(&settings);
  ASSERT_TRUE(collector.has_preset());
  collector.Collect(first);
  collector.Collect(second);
  EXPECT_EQ(3u, collector.collection_size());
  collector.Finish();
  EXPECT_FALSE(collector.has_preset());
  EXPECT_EQ(0u, collector.collection_size());
  EXPECT_EQ(4u, collector.num_entries);
  ExpectEntry(collector, "a", "a", 100);
  ExpectEntry(collector, "b", "b", 40);
  ExpectEntry(collector, "ab", "a b", 30);
  ExpectEntry(collector, "ba", "b a", 40);
}

}  // namespace
#endif
