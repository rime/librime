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

TEST(RimeKeyTableTest, KeycodeConvertion) {
  EXPECT_EQ(RIME_CONTROL_MASK, GetModifierByName("Control"));
  EXPECT_EQ(0, GetModifierByName("abracadabra"));
  EXPECT_EQ(0, GetModifierByName("control"));
  EXPECT_STREQ("Control", GetModifierName(RIME_CONTROL_MASK));
  EXPECT_STREQ("Release", GetModifierName(RIME_RELEASE_MASK));
  EXPECT_STREQ("Control", GetModifierName(RIME_CONTROL_MASK | RIME_RELEASE_MASK));
}

TEST(RimeKeyTableTest, ModifierConvertion) {
  EXPECT_EQ(XK_A, GetKeycodeByName("A"));
  EXPECT_EQ(XK_z, GetKeycodeByName("z"));
  EXPECT_EQ(XK_0, GetKeycodeByName("0"));
  EXPECT_EQ(XK_grave, GetKeycodeByName("grave"));
  EXPECT_EQ(XK_VoidSymbol, GetKeycodeByName("abracadabra"));
  EXPECT_EQ(XK_VoidSymbol, GetKeycodeByName("Control+c"));
  EXPECT_STREQ("a", GetKeyName(XK_a));
  EXPECT_STREQ("space", GetKeyName(XK_space));
  EXPECT_STREQ("0xfffe", GetKeyName(0xfffe));
  EXPECT_STREQ("0xfffffe", GetKeyName(0xfffffe));
}
