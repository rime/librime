//
// Copyright RIME Developers
// Distributed under the BSD License
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
  Segmentor::Component* component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component != NULL);
  the<Engine> engine(Engine::Create());
  the<Segmentor> segmentor(component->Create(engine.get()));
  ASSERT_TRUE(bool(segmentor));
  Segmentation segmentation;
  segmentation.Reset("3.1415926");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_FALSE(segmentation.HasFinishedSegmentation());
  ASSERT_EQ(0, segmentation.size());
}

TEST(AbcSegmentorTest, FullMatch) {
  Segmentor::Component* component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component != NULL);
  the<Engine> engine(Engine::Create());
  the<Segmentor> segmentor(component->Create(engine.get()));
  ASSERT_TRUE(bool(segmentor));
  Segmentation segmentation;
  segmentation.Reset("zyxwvutsrqponmlkjihgfedcba");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_TRUE(segmentation.HasFinishedSegmentation());
  ASSERT_EQ(1, segmentation.size());
  EXPECT_EQ(0, segmentation[0].start);
  EXPECT_EQ(26, segmentation[0].end);
  EXPECT_GE(1U, segmentation[0].tags.size());
}

TEST(AbcSegmentorTest, PrefixMatch) {
  Segmentor::Component* component = Segmentor::Require("abc_segmentor");
  ASSERT_TRUE(component != NULL);
  the<Engine> engine(Engine::Create());
  the<Segmentor> segmentor(component->Create(engine.get()));
  ASSERT_TRUE(bool(segmentor));
  Segmentation segmentation;
  segmentation.Reset("abcdefg.1415926");
  bool goon = segmentor->Proceed(&segmentation);
  EXPECT_TRUE(goon);
  EXPECT_FALSE(segmentation.HasFinishedSegmentation());
  ASSERT_EQ(1, segmentation.size());
  EXPECT_EQ(0, segmentation[0].start);
  EXPECT_EQ(7, segmentation[0].end);
  EXPECT_GE(1U, segmentation[0].tags.size());
}
