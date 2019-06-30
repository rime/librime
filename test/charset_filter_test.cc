//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/gear/charset_filter.h>
#include <rime/translation.h>

using namespace rime;


TEST(RimeCharsetFilterTest, FilterText) {
  EXPECT_TRUE(CharsetFilter::FilterText("荣", "unkown"));
  EXPECT_TRUE(CharsetFilter::FilterText("👋", "unkown"));

  EXPECT_TRUE(CharsetFilter::FilterText("Hello", "utf8"));
  EXPECT_TRUE(CharsetFilter::FilterText("荣", "utf8"));
  EXPECT_TRUE(CharsetFilter::FilterText("鎔", "utf8"));
  EXPECT_TRUE(CharsetFilter::FilterText("𤘺", "utf8"));
  EXPECT_TRUE(CharsetFilter::FilterText("👋", "utf8"));
  EXPECT_TRUE(CharsetFilter::FilterText("荣👋", "utf8"));

  EXPECT_TRUE(CharsetFilter::FilterText("Hello", "gbk"));
  EXPECT_TRUE(CharsetFilter::FilterText("荣", "gbk"));
  EXPECT_TRUE(CharsetFilter::FilterText("鎔", "gbk"));
  EXPECT_FALSE(CharsetFilter::FilterText("𤘺", "gbk"));
  EXPECT_FALSE(CharsetFilter::FilterText("👋", "gbk"));
  EXPECT_FALSE(CharsetFilter::FilterText("荣👋", "gbk"));

  EXPECT_TRUE(CharsetFilter::FilterText("Hello", "gb2312"));
  EXPECT_TRUE(CharsetFilter::FilterText("荣", "gb2312"));
  EXPECT_FALSE(CharsetFilter::FilterText("鎔", "gb2312"));
  EXPECT_FALSE(CharsetFilter::FilterText("𤘺", "gb2312"));
  EXPECT_FALSE(CharsetFilter::FilterText("👋", "gb2312"));

  EXPECT_TRUE(CharsetFilter::FilterText("👋", "gbk+emoji"));
  EXPECT_FALSE(CharsetFilter::FilterText("荣👋", "gbk+emoji"));

  EXPECT_TRUE(CharsetFilter::FilterText("👋", "gb2312+emoji"));
  EXPECT_FALSE(CharsetFilter::FilterText("荣👋", "gb2312+emoji"));
}

