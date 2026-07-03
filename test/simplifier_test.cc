//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <algorithm>
#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/gear/opencc.h>
#include <rime/gear/simplifier.h>

using namespace rime;

// ── OpenccTest ──────────────────────────────────────────────────────
// Integration tests exercising rime::Opencc with real dict files under
// share/opencc/. These serve as a regression safety net for future OpenCC
// upgrades. RIME_OPENCC_DICT_DIR is injected by CMake at compile time.

static const path kOpenccDir{RIME_OPENCC_DICT_DIR};

// Returns true if |forms| contains |s|.
static bool Contains(const vector<string>& forms, const string& s) {
  return std::find(forms.begin(), forms.end(), s) != forms.end();
}

TEST(OpenccTest, ConvertWord_ExactMatch) {
  // 裡 (traditional "inside") maps to exactly one simplified form: 里.
  Opencc oc(kOpenccDir / "t2s.json");
  vector<string> forms;
  EXPECT_TRUE(oc.ConvertWord("裡", &forms));
  ASSERT_EQ(1u, forms.size());
  EXPECT_EQ("里", forms[0]);
}

TEST(OpenccTest, ConvertWord_MultipleValues) {
  // 里 (simplified) expands to multiple traditional forms in s2t:
  //   裏 (traditional "inside"), 里 (unit of distance), 哩 (phonetic).
  Opencc oc(kOpenccDir / "s2t.json");
  vector<string> forms;
  EXPECT_TRUE(oc.ConvertWord("里", &forms));
  EXPECT_GE(forms.size(), 2u);
  EXPECT_TRUE(Contains(forms, "裏"));
  EXPECT_TRUE(Contains(forms, "里"));
}

TEST(OpenccTest, ConvertWord_NoMatch) {
  // ASCII has no dict entry; ConvertWord returns false.
  Opencc oc(kOpenccDir / "t2s.json");
  vector<string> forms;
  EXPECT_FALSE(oc.ConvertWord("abc", &forms));
}

TEST(OpenccTest, ConvertWord_ChainExpansion) {
  // s2twp chains STCharacters → TWVariants (among others).
  //   STCharacters: 里 → {裏, 里, 哩}
  //   TWVariants:   裏 → 裡   (Taiwan prefers 裡 over the mainland 裏)
  // After the chain, 裡 is present and 裏 is gone — evidence that results
  // accumulate correctly across dicts.
  Opencc oc(kOpenccDir / "s2twp.json");
  vector<string> forms;
  EXPECT_TRUE(oc.ConvertWord("里", &forms));
  EXPECT_TRUE(Contains(forms, "裡"));
  EXPECT_FALSE(Contains(forms, "裏"));
}

TEST(OpenccTest, ConvertWord_NoChange_ReturnsFalse) {
  // 里 is already simplified; t2s has no entry for it (t2s only maps
  // traditional chars that need conversion) → returns false.
  Opencc oc(kOpenccDir / "t2s.json");
  vector<string> forms;
  EXPECT_FALSE(oc.ConvertWord("里", &forms));
}

TEST(OpenccTest, ConvertText_FullSentence) {
  Opencc oc(kOpenccDir / "t2s.json");
  string out;
  EXPECT_TRUE(oc.ConvertText("電話號碼", &out));
  EXPECT_EQ("电话号码", out);
}

TEST(OpenccTest, ConvertText_NoChange_ReturnsFalse) {
  Opencc oc(kOpenccDir / "t2s.json");
  string out;
  EXPECT_FALSE(oc.ConvertText("电话号码", &out));
}

// ── 内存 / 記憶體: phrase-level vs char-level across four configs ─────
// These tests document the difference between configs with phrase
// dictionaries (s2twp, tw2sp) and those without (s2tw, t2s).

TEST(OpenccTest, ConvertWord_PhraseEntry_s2twp) {
  // s2twp has a whole-phrase entry: 内存 → 記憶體
  Opencc oc(kOpenccDir / "s2twp.json");
  vector<string> forms;
  EXPECT_TRUE(oc.ConvertWord("内存", &forms));
  ASSERT_EQ(1u, forms.size());
  EXPECT_EQ("記憶體", forms[0]);
}

TEST(OpenccTest, ConvertWord_NoPhraseEntry_s2tw) {
  // s2tw has no phrase entry for 内存; only converts individual characters
  Opencc oc(kOpenccDir / "s2tw.json");
  vector<string> forms;
  EXPECT_FALSE(oc.ConvertWord("内存", &forms));
}

TEST(OpenccTest, ConvertText_CharLevel_s2tw) {
  // s2tw converts 内→內 char-by-char; 存 has no traditional variant
  Opencc oc(kOpenccDir / "s2tw.json");
  string out;
  EXPECT_TRUE(oc.ConvertText("内存", &out));
  EXPECT_EQ("內存", out);
}

TEST(OpenccTest, ConvertWord_PhraseEntry_tw2sp) {
  // tw2sp has the reverse phrase entry: 記憶體 → 内存
  Opencc oc(kOpenccDir / "tw2sp.json");
  vector<string> forms;
  EXPECT_TRUE(oc.ConvertWord("記憶體", &forms));
  ASSERT_EQ(1u, forms.size());
  EXPECT_EQ("内存", forms[0]);
}

TEST(OpenccTest, ConvertText_CharLevel_t2s) {
  // t2s has no phrase entry for 記憶體; converts each char individually
  Opencc oc(kOpenccDir / "t2s.json");
  string out;
  EXPECT_TRUE(oc.ConvertText("記憶體", &out));
  EXPECT_EQ("记忆体", out);
}

TEST(OpenccTest, ConvertText_s2twp_NoMechanicalReplacement) {
  // "内存" alone is correctly substituted → "記憶體" (RAM).
  // Embedded in the classical line "海内存知己" (Wang Bo, 王勃), the same
  // two characters mean "within the seas" — a different word entirely.
  // OpenCC's segmentation must not mechanically replace it with "記憶體".
  Opencc oc(kOpenccDir / "s2twp.json");
  string out;
  oc.ConvertText("内存", &out);
  EXPECT_EQ("記憶體", out);
  oc.ConvertText("海内存知己", &out);
  EXPECT_EQ("海內存知己", out);
}

TEST(OpenccTest, Initialize_InvalidConfigPath) {
  // Bad path: Initialize() catches the exception, leaves converter_ null.
  // All three public methods must return false without crashing.
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

  bool ConvertWord(const string& /*text*/, vector<string>* forms) override {
    if (word_forms_.empty())
      return false;
    *forms = word_forms_;
    return true;
  }

  bool RandomConvertText(const string& /*text*/, string* simplified) override {
    if (random_result_.empty())
      return false;
    *simplified = random_result_;
    return true;
  }

  bool ConvertText(const string& /*text*/, string* simplified) override {
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

TEST_F(SimplifierConvertTest,
       ConvertWord_UnchangedFormPushesOriginalCandidate) {
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
  // word_forms_ is empty → ConvertWord returns false → falls back to
  // ConvertText.
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
