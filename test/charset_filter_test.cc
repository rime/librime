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
  Ticket ticket;
  CharsetFilter filter (ticket);
  EXPECT_TRUE(filter.FilterText("Hello", "utf8"));
  EXPECT_TRUE(filter.FilterText("荣", "utf8"));
  EXPECT_TRUE(filter.FilterText("𤘺", "utf8"));
  EXPECT_TRUE(filter.FilterText("👋", "utf8"));
  EXPECT_TRUE(filter.FilterText("👋", "utf8"));
  EXPECT_TRUE(filter.FilterText("荣👋", "utf8"));
  EXPECT_TRUE(filter.FilterText("鎔", "gbk"));
  EXPECT_TRUE(filter.FilterText("𤘺", "utf8"));

  EXPECT_TRUE(filter.FilterText("Hello", "gbk"));
  EXPECT_TRUE(filter.FilterText("荣", "gbk"));
  EXPECT_FALSE(filter.FilterText("𤘺", "gbk"));
  EXPECT_FALSE(filter.FilterText("👋", "gbk"));
  EXPECT_TRUE(filter.FilterText("👋", "gbk+emoji"));
  EXPECT_FALSE(filter.FilterText("荣👋", "gbk+emoji"));
  EXPECT_TRUE(filter.FilterText("鎔", "gbk"));
  EXPECT_FALSE(filter.FilterText("𤘺", "gbk"));

  EXPECT_TRUE(filter.FilterText("Hello", "gb2312"));
  EXPECT_TRUE(filter.FilterText("荣", "gb2312"));
  EXPECT_FALSE(filter.FilterText("𤘺", "gb2312"));
  EXPECT_FALSE(filter.FilterText("👋", "gb2312"));
  EXPECT_TRUE(filter.FilterText("👋", "gb2312+emoji"));
  EXPECT_FALSE(filter.FilterText("荣👋", "gb2312+emoji"));
  EXPECT_FALSE(filter.FilterText("鎔", "gb2312"));
  EXPECT_FALSE(filter.FilterText("𤘺", "gb2312"));
}

