//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-07-21 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/algo/encoder.h>

TEST(RimeEncoderTest, Settings) {
  rime::Config config;
  config["encoder"]["rules"][0]["length_equal"] = 2;
  config["encoder"]["rules"][0]["formula"] = "AaAzBaBz";
  config["encoder"]["rules"][1]["length_equal"] = 3;
  config["encoder"]["rules"][1]["formula"] = "AaBaCaBz";
  config["encoder"]["rules"][2]["length_in_range"][0] = 4;
  config["encoder"]["rules"][2]["length_in_range"][1] = 9;
  config["encoder"]["rules"][2]["formula"] = "AaBaCaZz";
  rime::TableEncoder encoder;
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.loaded());
  const auto& rules(encoder.encoding_rules());
  ASSERT_EQ(3, rules.size());
  EXPECT_EQ(2, rules[0].min_word_length);
  EXPECT_EQ(2, rules[0].max_word_length);
  EXPECT_EQ(4, rules[2].min_word_length);
  EXPECT_EQ(9, rules[2].max_word_length);
  ASSERT_EQ(4, rules[0].coords.size());
  EXPECT_EQ(0, rules[0].coords[0].char_index);
  EXPECT_EQ(0, rules[0].coords[0].code_index);
  EXPECT_EQ(0, rules[0].coords[1].char_index);
  EXPECT_EQ(-1, rules[0].coords[1].code_index);
  EXPECT_EQ(1, rules[0].coords[2].char_index);
  EXPECT_EQ(0, rules[0].coords[2].code_index);
  EXPECT_EQ(1, rules[0].coords[3].char_index);
  EXPECT_EQ(-1, rules[0].coords[3].code_index);
  ASSERT_EQ(4, rules[2].coords.size());
  EXPECT_EQ(0, rules[2].coords[0].char_index);
  EXPECT_EQ(0, rules[2].coords[0].code_index);
  EXPECT_EQ(1, rules[2].coords[1].char_index);
  EXPECT_EQ(0, rules[2].coords[1].code_index);
  EXPECT_EQ(2, rules[2].coords[2].char_index);
  EXPECT_EQ(0, rules[2].coords[2].code_index);
  EXPECT_EQ(-1, rules[2].coords[3].char_index);
  EXPECT_EQ(-1, rules[2].coords[3].code_index);
}

TEST(RimeEncoderTest, ExcludePatterns) {
  rime::Config config;
  config["encoder"]["exclude_patterns"][0] = "^x.*$";
  rime::TableEncoder encoder;
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.IsCodeExcluded("x"));
  EXPECT_TRUE(encoder.IsCodeExcluded("xyz"));
  EXPECT_FALSE(encoder.IsCodeExcluded("XYZ"));
}

TEST(RimeEncoderTest, Encode) {
  rime::TableEncoder encoder;
  rime::Config config;
  rime::RawCode c0, c1;
  c0.FromString("abc def");
  c1.FromString("abc def ghi");
  rime::string result;
  // case 1
  config["encoder"]["rules"][0]["length_equal"] = 2;
  config["encoder"]["rules"][0]["formula"] = "AaAbBaBb";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c0, &result));
  EXPECT_EQ("abde", result);
  // case 2
  config["encoder"]["rules"][1]["length_in_range"][0] = 3;
  config["encoder"]["rules"][1]["length_in_range"][1] = 5;
  config["encoder"]["rules"][1]["formula"] = "AaAzBaBzCaCz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c1, &result));
  EXPECT_EQ("acdfgi", result);
  // case 3
  config["encoder"]["rules"][0]["formula"] = "AaAzBaBzCaCz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c0, &result));
  EXPECT_EQ("acdf", result);
  // case 4
  config["encoder"]["rules"][0]["formula"] = "AaAbZyZz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c0, &result));
  EXPECT_EQ("abef", result);
  // case 5
  config["encoder"]["rules"][0]["formula"] = "AaAaBbBbZzZz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c0, &result));
  EXPECT_EQ("aaeeff", result);
  // case 6
  config["encoder"]["rules"][1]["formula"] = "AzAzByByZaZa";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c1, &result));
  EXPECT_EQ("cceegg", result);
  // case 7
  config["encoder"]["rules"][0]["formula"] = "AaBaYaZaZz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c0, &result));
  EXPECT_EQ("adf", result);
}

TEST(RimeEncoderTest, TailAnchor) {
  rime::TableEncoder encoder;
  rime::Config config;
  config["encoder"]["rules"][0]["length_equal"] = 3;
  config["encoder"]["tail_anchor"] = "'";
  rime::RawCode c;
  c.FromString("zyx'wvu'tsr qpo'nmlk'jih gfedcba");
  rime::string result;
  // case 1
  config["encoder"]["rules"][0]["formula"] = "AaAzBaBzCaCz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c, &result));
  EXPECT_EQ("zxqoga", result);
  // case 2
  config["encoder"]["rules"][0]["formula"] = "AaAbAcAzBwBxByBz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c, &result));
  EXPECT_EQ("zyxuqpo", result);
  // case 3
  config["encoder"]["rules"][0]["formula"] = "AaAbAcAdAzBaBxByBz";
  encoder.LoadSettings(&config);
  EXPECT_TRUE(encoder.Encode(c, &result));
  EXPECT_EQ("zyxwuqpo", result);
}
