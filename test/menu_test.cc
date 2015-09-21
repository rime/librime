//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/menu.h>
#include <rime/translation.h>

using namespace rime;

class TranslationAlpha : public Translation {
 public:
  bool Next() {
    if (exhausted())
      return false;
    set_exhausted(true);
    return true;
  }
  an<Candidate> Peek() {
    if (exhausted())
      return nullptr;
    return New<SimpleCandidate>("alpha", 0, 5, "Alpha");
  }
};

class TranslationBeta : public Translation {
 public:
  TranslationBeta() : cursor_(0) {
    candies_.push_back(New<SimpleCandidate>("beta", 0, 4, "Beta-1"));
    candies_.push_back(New<SimpleCandidate>("beta", 0, 4, "Beta-2"));
    candies_.push_back(New<SimpleCandidate>("beta", 0, 4, "Beta-3"));
  }

  bool Next() {
    if (exhausted())
      return false;
    if (++cursor_ >= candies_.size())
      set_exhausted(true);
    return true;
  }

  an<Candidate> Peek() {
    if (exhausted())
      return nullptr;
    return candies_[cursor_];
  }

 private:
  vector<of<Candidate>> candies_;
  size_t cursor_;
};

TEST(RimeMenuTest, RecipeAlphaBeta) {
  Menu menu;
  auto alpha = New<TranslationAlpha>();
  auto beta = New<TranslationBeta>();
  menu.AddTranslation(alpha);
  menu.AddTranslation(beta);
  // explicit call to Menu::Prepare() is not necessary
  menu.Prepare(2);
  the<Page> page(menu.CreatePage(5, 0));
  ASSERT_TRUE(bool(page));
  EXPECT_EQ(5, page->page_size);
  EXPECT_EQ(0, page->page_no);
  EXPECT_TRUE(page->is_last_page);
  ASSERT_EQ(4, page->candidates.size());
  EXPECT_EQ("alpha", page->candidates[0]->type());
  EXPECT_EQ("Alpha", page->candidates[0]->text());
  EXPECT_EQ("beta", page->candidates[1]->type());
  EXPECT_EQ("Beta-1", page->candidates[1]->text());
  EXPECT_EQ("Beta-3", page->candidates[3]->text());
  the<Page> no_more_page(menu.CreatePage(5, 1));
  EXPECT_FALSE(bool(no_more_page));
}
