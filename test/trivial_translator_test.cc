// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/candidate.h>
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
  const std::string test_input("yiqianerbaisanshisiabc");
  Segment segment;
  segment.start = 0;
  segment.end = test_input.length();
  segment.tags.insert("abc");
  scoped_ptr<Translation> translation(translator->Query(test_input, segment));
  ASSERT_TRUE(translation);
  ASSERT_FALSE(translation->exhausted());
  shared_ptr<Candidate> candidate = translation->Next();
  ASSERT_TRUE(candidate);
  EXPECT_EQ("abc", candidate->type());
  EXPECT_EQ("一千二百三十四abc", candidate->text());
  EXPECT_EQ(segment.start, candidate->start());
  EXPECT_EQ(segment.end, candidate->end());
} 
