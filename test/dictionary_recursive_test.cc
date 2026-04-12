//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dictionary.h>

namespace {

rime::the<rime::Dictionary> BuildDictionaryOrNull(const std::string& name) {
  rime::the<rime::Dictionary> dict(new rime::Dictionary(
      name, {},
      {rime::New<rime::Table>(rime::path{name + ".table.bin"})},
      rime::New<rime::Prism>(rime::path{name + ".prism.bin"})));

  dict->Remove();
  rime::DictCompiler compiler(dict.get());
  if (!compiler.Compile(rime::path())) {
    return nullptr;
  }
  if (!dict->Load()) {
    return nullptr;
  }
  return dict;
}

void ExpectLookupFound(rime::Dictionary* dict,
                       const std::string& code,
                       const std::string& text) {
  ASSERT_TRUE(dict);
  ASSERT_TRUE(dict->loaded());

  rime::DictEntryIterator it;
  dict->LookupWords(&it, code, false);
  ASSERT_FALSE(it.exhausted()) << "expected code '" << code << "' to be found";
  ASSERT_TRUE(bool(it.Peek()));
  EXPECT_EQ(text, it.Peek()->text);
}

void ExpectLookupNotFound(rime::Dictionary* dict, const std::string& code) {
  ASSERT_TRUE(dict);
  ASSERT_TRUE(dict->loaded());

  rime::DictEntryIterator it;
  dict->LookupWords(&it, code, false);
  EXPECT_TRUE(it.exhausted()) << "expected code '" << code << "' to be absent";
}

class RimeRecursiveImportDictionaryTest : public ::testing::Test {
 public:
  void SetUp() override {
    if (!dict_) {
      dict_ = BuildDictionaryOrNull("dictionary_recursive_test");
    }
  }

  void TearDown() override { dict_.reset(); }

 protected:
  static rime::the<rime::Dictionary> dict_;
};

rime::the<rime::Dictionary> RimeRecursiveImportDictionaryTest::dict_;

class RimeNonRecursiveImportDictionaryTest : public ::testing::Test {
 public:
  void SetUp() override {
    if (!dict_) {
      dict_ = BuildDictionaryOrNull("dictionary_nonrecursive_test");
    }
  }

  void TearDown() override { dict_.reset(); }

 protected:
  static rime::the<rime::Dictionary> dict_;
};

rime::the<rime::Dictionary> RimeNonRecursiveImportDictionaryTest::dict_;

TEST_F(RimeRecursiveImportDictionaryTest, Ready) {
  ASSERT_TRUE(dict_);
  EXPECT_TRUE(dict_->loaded());
}

TEST_F(RimeRecursiveImportDictionaryTest, RecursiveImportChainLookup) {
  ASSERT_TRUE(dict_);
  ASSERT_TRUE(dict_->loaded());

  // root dictionary entry
  ExpectLookupFound(dict_.get(), "a", "a");

  // directly imported dictionary entry
  ExpectLookupFound(dict_.get(), "b", "b");

  // transitively imported dictionary entry
  ExpectLookupFound(dict_.get(), "c", "c");
}

TEST_F(RimeNonRecursiveImportDictionaryTest, Ready) {
  ASSERT_TRUE(dict_);
  EXPECT_TRUE(dict_->loaded());
}

TEST_F(RimeNonRecursiveImportDictionaryTest, LegacyOneLevelImportOnly) {
  ASSERT_TRUE(dict_);
  ASSERT_TRUE(dict_->loaded());

  // root dictionary entry
  ExpectLookupFound(dict_.get(), "na", "na");

  // directly imported dictionary entry
  ExpectLookupFound(dict_.get(), "nb", "nb");

  // transitively imported dictionary entry should NOT be visible
  ExpectLookupNotFound(dict_.get(), "nc");
}

TEST(RimeDictCompilerRecursiveImportTest, RecursiveImportCycleFails) {
  rime::the<rime::Dictionary> dict(new rime::Dictionary(
      "dictionary_cycle_test", {},
      {rime::New<rime::Table>(rime::path{"dictionary_cycle_test.table.bin"})},
      rime::New<rime::Prism>(rime::path{"dictionary_cycle_test.prism.bin"})));

  dict->Remove();
  rime::DictCompiler compiler(dict.get());
  EXPECT_FALSE(compiler.Compile(rime::path()));
}

}  // namespace
