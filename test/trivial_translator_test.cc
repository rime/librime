// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/translator.h>

using namespace rime;

TEST(TrivialTranslatorTest, Query) {
  // make sure the component has been registered
  Translator::Component *component =
      Translator::Require("trivial_translator");
  ASSERT_TRUE(component);
  // make sure the dict object has been created
  scoped_ptr<Translator> translator(component->Create(NULL));
  ASSERT_TRUE(translator);
  // lookup test
  const std::string test_input("test");
  Segment segment;
  segment.start = 0;
  segment.end = test_input.length();
  segment.tags.insert("abc");
  scoped_ptr<Translation> translation(translator->Query(test_input, segment));
  ASSERT_TRUE(translation);
  std::string result = translation->result();
  EXPECT_EQ(test_input, result);
}

