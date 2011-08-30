// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-07 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/key_table.h>

TEST(RimeKeyTableTest, KeycodeLookup) {
  EXPECT_EQ(kControlMask, RimeGetModifierByName("Control"));
  EXPECT_EQ(0, RimeGetModifierByName("abracadabra"));
  EXPECT_EQ(0, RimeGetModifierByName("control"));
  EXPECT_STREQ("Control", RimeGetModifierName(kControlMask));
  EXPECT_STREQ("Release", RimeGetModifierName(kReleaseMask));
  EXPECT_STREQ("Control", RimeGetModifierName(kControlMask | kReleaseMask));
}

TEST(RimeKeyTableTest, ModifierLookup) {
  EXPECT_EQ(XK_A, RimeGetKeycodeByName("A"));
  EXPECT_EQ(XK_z, RimeGetKeycodeByName("z"));
  EXPECT_EQ(XK_0, RimeGetKeycodeByName("0"));
  EXPECT_EQ(XK_grave, RimeGetKeycodeByName("grave"));
  EXPECT_EQ(XK_VoidSymbol, RimeGetKeycodeByName("abracadabra"));
  EXPECT_EQ(XK_VoidSymbol, RimeGetKeycodeByName("Control+c"));
  EXPECT_STREQ("a", RimeGetKeyName(XK_a));
  EXPECT_STREQ("space", RimeGetKeyName(XK_space));
  EXPECT_STREQ(NULL, RimeGetKeyName(0xfffe));
  EXPECT_STREQ(NULL, RimeGetKeyName(0xfffffe));
}
