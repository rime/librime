//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/gear/opencc.h>
#include <rime/gear/simplifier.h>

using namespace rime;

// ── OpenccTest ──────────────────────────────────────────────────────
// Integration tests that exercise rime::Opencc against real dict files.
// RIME_OPENCC_DICT_DIR is injected by CMake at compile time.

static const path kOpenccDir{RIME_OPENCC_DICT_DIR};

TEST(OpenccTest, ConvertText_TraditionalToSimplified) {
  Opencc oc(kOpenccDir / "t2s.json");
  string out;
  EXPECT_TRUE(oc.ConvertText("裡", &out));
  EXPECT_EQ("里", out);
}

TEST(OpenccTest, ConvertText_AlreadySimplified_ReturnsFalse) {
  Opencc oc(kOpenccDir / "t2s.json");
  string out;
  // "里" is already simplified; t2s leaves it unchanged → returns false
  EXPECT_FALSE(oc.ConvertText("里", &out));
}

TEST(OpenccTest, ConvertWord_ReturnsVariantForms) {
  Opencc oc(kOpenccDir / "s2t.json");
  vector<string> forms;
  // "里" has multiple traditional variants in the s2t dictionaries
  EXPECT_TRUE(oc.ConvertWord("里", &forms));
  EXPECT_FALSE(forms.empty());
}

TEST(OpenccTest, ConvertWord_NoExactDictMatch_ReturnsFalse) {
  Opencc oc(kOpenccDir / "t2s.json");
  vector<string> forms;
  // ASCII characters have no entry in the t2s dictionary
  EXPECT_FALSE(oc.ConvertWord("abc", &forms));
}

TEST(OpenccTest, InvalidConfigPath_AllMethodsReturnFalse) {
  Opencc oc(path{"/nonexistent/invalid.json"});
  string out;
  EXPECT_FALSE(oc.ConvertText("裡", &out));
  vector<string> forms;
  EXPECT_FALSE(oc.ConvertWord("裡", &forms));
  EXPECT_FALSE(oc.RandomConvertText("裡", &out));
}

// ── FakeOpencc ──────────────────────────────────────────────────────

class FakeOpencc : public Opencc {
 public:
  FakeOpencc() : Opencc(path{}) {}

  bool ConvertWord(const string& /*text*/,
                   vector<string>* forms) override {
    if (word_forms_.empty())
      return false;
    *forms = word_forms_;
    return true;
  }

  bool RandomConvertText(const string& /*text*/,
                         string* simplified) override {
    if (random_result_.empty())
      return false;
    *simplified = random_result_;
    return true;
  }

  bool ConvertText(const string& /*text*/,
                   string* simplified) override {
    if (text_result_.empty())
      return false;
    *simplified = text_result_;
    return true;
  }

  vector<string> word_forms_;
  string text_result_;
  string random_result_;
};

// ── SimplifierConvertTest ────────────────────────────────────────────
// Unit tests for Simplifier::Convert() using FakeOpencc.
// Simplifier is constructed with a null-engine Ticket, so no schema/config
// is read and all settings remain at their defaults.

class SimplifierConvertTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fake_ = New<FakeOpencc>();
    Ticket ticket;
    ticket.name_space = "simplifier";
    simplifier_.reset(new Simplifier(ticket, fake_));
  }

  an<FakeOpencc> fake_;
  the<Simplifier> simplifier_;
};

TEST_F(SimplifierConvertTest, ConvertWord_PushesConvertedForm) {
  fake_->word_forms_ = {"里"};
  auto c = New<SimpleCandidate>("word", 0, 1, "裡");
  CandidateQueue result;
  EXPECT_TRUE(simplifier_->Convert(c, &result));
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("simplified", result.front()->type());
  EXPECT_EQ("里", result.front()->text());
}

TEST_F(SimplifierConvertTest, ConvertWord_UnchangedFormPushesOriginalCandidate) {
  // First form matches original → original candidate pushed directly.
  // Second form differs → ShadowCandidate pushed.
  fake_->word_forms_ = {"裡", "里"};
  auto c = New<SimpleCandidate>("word", 0, 1, "裡");
  CandidateQueue result;
  EXPECT_TRUE(simplifier_->Convert(c, &result));
  ASSERT_EQ(2u, result.size());
  auto it = result.begin();
  EXPECT_EQ("word", (*it)->type());
  EXPECT_EQ("裡", (*it)->text());
  ++it;
  EXPECT_EQ("simplified", (*it)->type());
  EXPECT_EQ("里", (*it)->text());
}

TEST_F(SimplifierConvertTest, ConvertWord_Fails_FallsBackToConvertText) {
  // word_forms_ is empty → ConvertWord returns false → falls back to ConvertText.
  fake_->text_result_ = "里";
  auto c = New<SimpleCandidate>("word", 0, 1, "裡");
  CandidateQueue result;
  EXPECT_TRUE(simplifier_->Convert(c, &result));
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("simplified", result.front()->type());
  EXPECT_EQ("里", result.front()->text());
}

TEST_F(SimplifierConvertTest, AllConversionsFail_ReturnsFalse) {
  // Both word_forms_ and text_result_ are empty → Convert returns false.
  auto c = New<SimpleCandidate>("word", 0, 1, "裡");
  CandidateQueue result;
  EXPECT_FALSE(simplifier_->Convert(c, &result));
  EXPECT_TRUE(result.empty());
}
