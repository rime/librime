// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-07 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/key_event.h>

using namespace rime;

TEST(RimeKeyEventTest, Null) {
  // TODO:
}

TEST(RimeKeyEventTest, KeyName) {
  KeyEvent comma(XK_comma, 0);
  EXPECT_STREQ("comma", comma.repr().c_str());
  KeyEvent period(XK_period, 0);
  EXPECT_STREQ("period", period.repr().c_str());
}

TEST(RimeKeyEventTest, HexKeyName) {
  KeyEvent ke_fffe(0xfffe, 0);
  EXPECT_STREQ("0xfffe", ke_fffe.repr().c_str());
  KeyEvent ke_fffffe(0xfffffe, 0);
  EXPECT_STREQ("0xfffffe", ke_fffffe.repr().c_str());
}

TEST(RimeKeyEventTest, BadKeyName) {
  KeyEvent bad_ke(0x1000000, 0);
  EXPECT_STREQ("(unknown)", bad_ke.repr().c_str());
}

TEST(RimeKeySequenceTest, Null) {
  // TODO:
}
