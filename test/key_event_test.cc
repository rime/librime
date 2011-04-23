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

TEST(RimeKeyEventTest, ModifiedKeyEvent) {
  KeyEvent ke(XK_A, RIME_SHIFT_MASK | RIME_CONTROL_MASK | RIME_RELEASE_MASK);
  EXPECT_TRUE(ke.shift());
  EXPECT_FALSE(ke.alt());
  EXPECT_TRUE(ke.ctrl());
  EXPECT_TRUE(ke.release());
}

TEST(RimeKeyEventTest, ModifiedKeyEventRepresentation) {
  KeyEvent ctrl_a(XK_a, RIME_CONTROL_MASK);
  EXPECT_STREQ("Control+a", ctrl_a.repr().c_str());
  KeyEvent less_keyup(XK_less, RIME_SHIFT_MASK | RIME_RELEASE_MASK);
  EXPECT_STREQ("Shift+Release+less", less_keyup.repr().c_str());
}

TEST(RimeKeyEventTest, ParseKeyEventRepresentation) {
  KeyEvent ke;
  ASSERT_TRUE(ke.Parse("Shift+Control+Release+A"));
  EXPECT_EQ(XK_A, ke.keycode());
  EXPECT_TRUE(ke.shift());
  EXPECT_FALSE(ke.alt());
  EXPECT_TRUE(ke.ctrl());
  EXPECT_TRUE(ke.release());
}

TEST(RimeKeyEventTest, ConstructKeyEventFromRepresentation) {
  KeyEvent ke("Alt+F4");
  EXPECT_EQ(XK_F4, ke.keycode());
  EXPECT_TRUE(ke.alt());
  EXPECT_FALSE(ke.release());
}

TEST(RimeKeyEventTest, Equality) {
  KeyEvent ke0(XK_plus, 0);
  KeyEvent ke1("+");
  KeyEvent ke2("plus");
  EXPECT_TRUE(ke0 == ke1);
  EXPECT_TRUE(ke1 == ke2);
}

TEST(RimeKeySequenceTest, PlainString) {
  KeySequence ks("zyx123CBA");
  ASSERT_EQ(9, ks.size());
  EXPECT_EQ(XK_z, ks[0].keycode());
  EXPECT_FALSE(ks[0].release());
  // explanations:
  // Shift is not necessarily implied by uppercase letter 'A'
  // imagine that we may have customized a keyboard with a separate 'A' key
  // when the Shift modifier counts, we'll write "{Shift+A}" instead
  // a real life key sequence could even be like this:
  // "{Shift_L}{Shift+A}{Shift+Release+A}{Shift_L+Release}"
  // here we just focus on the information useful to the ime
  EXPECT_EQ(XK_A, ks[8].keycode());
  EXPECT_FALSE(ks[8].shift());
}

TEST(RimeKeySequenceTest, KeySequenceWithNamedKeys) {
  KeySequence ks("zyx 123{space}ABC{Return}");
  ASSERT_EQ(12, ks.size());
  EXPECT_EQ(XK_space, ks[3].keycode());
  EXPECT_EQ(XK_space, ks[7].keycode());
  EXPECT_EQ(XK_Return, ks[11].keycode());
}

TEST(RimeKeySequenceTest, KeySequenceWithModifiedKeys) {
  KeySequence ks("zyx 123{Shift+space}ABC{Control+Alt+Return}");
  ASSERT_EQ(12, ks.size());
  EXPECT_EQ(XK_space, ks[3].keycode());
  EXPECT_FALSE(ks[3].shift());
  EXPECT_FALSE(ks[3].release());
  EXPECT_EQ(XK_space, ks[7].keycode());
  EXPECT_TRUE(ks[7].shift());
  EXPECT_FALSE(ks[7].release());
  EXPECT_EQ(XK_Return, ks[11].keycode());
  EXPECT_FALSE(ks[11].shift());
  EXPECT_TRUE(ks[11].ctrl());
  EXPECT_TRUE(ks[11].alt());
  EXPECT_FALSE(ks[11].release());
}

TEST(RimeKeySequenceTest, Stringify) {
  KeySequence ks;
  ASSERT_TRUE(ks.Parse("z y,x."));
  ks.push_back(KeyEvent("{"));
  ks.push_back(KeyEvent("}"));
  EXPECT_STREQ("z{space}y{comma}x{period}{braceleft}{braceright}",
               ks.repr().c_str());
}
