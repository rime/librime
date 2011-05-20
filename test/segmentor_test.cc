// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-20 GONG Chen <chen.sst@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/engine.h>
#include <rime/segmentation.h>
#include <rime/segmentor.h>

using namespace rime;

TEST(AbcSegmentorTest, NoMatch) {
  Segmentor::Component *component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component);
  Engine engine;
  scoped_ptr<Segmentor> segmentor(component->Create(&engine));
  ASSERT_TRUE(segmentor);
  Segmentation segmentation("3.1415926");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_FALSE(segmentation.HasFinished());
  ASSERT_EQ(0, segmentation.segments().size());
}

TEST(AbcSegmentorTest, FullMatch) {
  Segmentor::Component *component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component);
  Engine engine;
  scoped_ptr<Segmentor> segmentor(component->Create(&engine));
  ASSERT_TRUE(segmentor);
  Segmentation segmentation("zyxwvutsrqponmlkjihgfedcba");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_TRUE(segmentation.HasFinished());
  ASSERT_EQ(1, segmentation.segments().size());
  EXPECT_EQ(0, segmentation.segments()[0].start);
  EXPECT_EQ(26, segmentation.segments()[0].end);
  EXPECT_GE(1, segmentation.segments()[0].tags.size());
}

TEST(AbcSegmentorTest, PrefixMatch) {
  Segmentor::Component *component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component);
  Engine engine;
  scoped_ptr<Segmentor> segmentor(component->Create(&engine));
  ASSERT_TRUE(segmentor);
  Segmentation segmentation("abcdefg.1415926");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_FALSE(segmentation.HasFinished());
  ASSERT_EQ(1, segmentation.segments().size());
  EXPECT_EQ(0, segmentation.segments()[0].start);
  EXPECT_EQ(7, segmentation.segments()[0].end);
  EXPECT_GE(1, segmentation.segments()[0].tags.size());
}
