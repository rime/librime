// encoding: utf-8
//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
// 2013-10-20 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/translator.h>

using namespace rime;

TEST(/*DISABLED_*/TrivialTranslatorTest, Query) {
  // make sure the component has been registered
  Translator::Component *component =
      Translator::Require("trivial_translator");
  ASSERT_TRUE(component != NULL);
  Ticket ticket;
  scoped_ptr<Translator> translator(component->Create(ticket));
  // make sure the dict object has been created
  ASSERT_TRUE(translator);
  // lookup test
  const std::string test_input("yiqianerbaisanshisi");
  // 一千二百三十四
  const std::string expected_output("\xe4\xb8\x80"
                                    "\xe5\x8d\x83"
                                    "\xe4\xba\x8c"
                                    "\xe7\x99\xbe"
                                    "\xe4\xb8\x89"
                                    "\xe5\x8d\x81"
                                    "\xe5\x9b\x9b");
  Segment segment;
  segment.start = 0;
  segment.end = test_input.length();
  segment.tags.insert("abc");
  shared_ptr<Translation> translation(translator->Query(test_input, segment));
  ASSERT_TRUE(translation);
  ASSERT_FALSE(translation->exhausted());
  shared_ptr<Candidate> candidate = translation->Peek();
  ASSERT_TRUE(bool(candidate));
  EXPECT_EQ("trivial", candidate->type());
  EXPECT_EQ(expected_output, candidate->text());
  EXPECT_EQ(segment.start, candidate->start());
  EXPECT_EQ(segment.end, candidate->end());
  EXPECT_TRUE(translation->Next());
}
