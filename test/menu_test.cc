// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
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
  shared_ptr<Candidate> Next() {
    if (exhausted())
      return shared_ptr<Candidate>();
    set_exhausted(true);
    return shared_ptr<Candidate>(
        new Candidate("alpha", "Alpha", "", 0, 5, 0));
  }
  shared_ptr<Candidate> Peek() const {
    if (exhausted())
      return shared_ptr<Candidate>();
    return shared_ptr<Candidate>(
        new Candidate("alpha", "Alpha", "", 0, 5, 0));
  }
};

class TranslationBeta : public Translation {
 public:
  TranslationBeta() : cursor_(0) {
    candies_.push_back(shared_ptr<Candidate>(
        new Candidate("beta", "Beta-1", "", 0, 4, 0)));
    candies_.push_back(shared_ptr<Candidate>(
        new Candidate("beta", "Beta-2", "", 0, 4, 0)));
    candies_.push_back(shared_ptr<Candidate>(
        new Candidate("beta", "Beta-3", "", 0, 4, 0)));
  }
  
  shared_ptr<Candidate> Next() {
    if (exhausted())
      return shared_ptr<Candidate>();
    if (cursor_ + 1 >= candies_.size())
      set_exhausted(true);
    return candies_[cursor_++];
  }
  
  shared_ptr<Candidate> Peek() const {
    if (exhausted())
      return shared_ptr<Candidate>();
    return candies_[cursor_];
  }
  
 private:
  std::vector<shared_ptr<Candidate> > candies_;
  int cursor_;
};

TEST(RimeMenuTest, RecipeAlphaBeta) {
  Menu menu;
  shared_ptr<Translation> alpha(new TranslationAlpha());
  shared_ptr<Translation> beta(new TranslationBeta());
  menu.AddTranslation(alpha);
  menu.AddTranslation(beta);
  // TODO:
  //menu.Prepare(5);
  //menu.GetCurrentPage();
}
