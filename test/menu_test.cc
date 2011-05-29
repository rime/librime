// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/menu.h>
#include <rime/translation.h>

using namespace rime;

class MockTranslation : public Translation {
 public:
  shared_ptr<Candidate> Next() {
    return shared_ptr<Candidate>();
  }
  shared_ptr<const Candidate> Peek() const {
    return shared_ptr<const Candidate>();
  }
};

TEST(RimeMenuTest, TestA) {
  Menu menu;
  shared_ptr<Translation> translation(new MockTranslation());
  menu.AddTranslation(translation);
}
