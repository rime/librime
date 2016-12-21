// encoding: utf-8
//
// Copyright RIME Developers
// Distributed under the BSD License
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
  auto component = Translator::Require("trivial_translator");
  ASSERT_TRUE(component != NULL);
  Ticket ticket;
  the<Translator> translator(component->Create(ticket));
  // make sure the dict object has been created
  ASSERT_TRUE(bool(translator));
  // lookup test
  const string test_input("yiqianerbaisanshisi");
  // 一千二百三十四
  const string expected_output("\xe4\xb8\x80"
                               "\xe5\x8d\x83"
                               "\xe4\xba\x8c"
                               "\xe7\x99\xbe"
                               "\xe4\xb8\x89"
                               "\xe5\x8d\x81"
                               "\xe5\x9b\x9b");
  Segment segment(0, test_input.length());
  segment.tags.insert("abc");
  auto translation = translator->Query(test_input, segment);
  ASSERT_TRUE(bool(translation));
  ASSERT_FALSE(translation->exhausted());
  auto candidate = translation->Peek();
  ASSERT_TRUE(bool(candidate));
  EXPECT_EQ("trivial", candidate->type());
  EXPECT_EQ(expected_output, candidate->text());
  EXPECT_EQ(segment.start, candidate->start());
  EXPECT_EQ(segment.end, candidate->end());
  EXPECT_TRUE(translation->Next());
}
