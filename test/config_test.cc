// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-04-06 Zou xu <zouivex@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>

using namespace rime;

class RimeConfigTest : public ::testing::Test {
 protected:
  RimeConfigTest() : component_(NULL), config_(NULL) {}

  virtual void SetUp() {
    component_ = new ConfigComponent("%s.yaml");
    config_ = component_->Create("config_test");
  }

  virtual void TearDown() {
    if (config_)
      delete config_;
    if (component_)
      delete component_;
  }

  Config::Component *component_;
  Config *config_;
};

TEST(RimeConfigComponentTest, RealCreationWorkflow) {
  // registration
  Registry &r = Registry::instance();
  r.Register("test_config", new ConfigComponent("%s.yaml"));
  // finding component
  Config::Component *cc = Config::Require("test_config");
  ASSERT_TRUE(cc != NULL);
  // creation
  scoped_ptr<Config> config(cc->Create("config_test"));
  EXPECT_TRUE(config);
  r.Unregister("test_config");
}

TEST(RimeConfigItemTest, NullItem) {
  ConfigItem item;
  EXPECT_EQ(ConfigItem::kNull, item.type());
}

TEST_F(RimeConfigTest, Config_IsNull) {
  bool is_null = config_->IsNull("terrans/tank");
  EXPECT_FALSE(is_null);

  is_null = config_->IsNull("protoss/tank");
  EXPECT_TRUE(is_null);
}

TEST_F(RimeConfigTest, Config_GetBool) {
  bool ret, value;
  ret = config_->GetBool("terrans/tank/seiged", &value);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(value);

  ret = config_->GetBool("zerg/lurker/burrowed", &value);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(value);
}

TEST_F(RimeConfigTest, Config_GetInt) {
  bool ret;
  int value;
  ret = config_->GetInt("terrans/supply/produced", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(28, value);

  ret = config_->GetInt("zerg/zergling/lost", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(1234, value);
}

TEST_F(RimeConfigTest, Config_GetDouble) {
  bool ret;
  double value;
  ret = config_->GetDouble("terrans/math/pi", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(3.1415926, value);

  ret = config_->GetDouble("protoss/battery/energy", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(10.111, value);
}

TEST_F(RimeConfigTest, Config_GetString) {
  bool ret;
  std::string value;
  ret = config_->GetString("protoss/residence", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Aiur", value);

  ret = config_->GetString("zerg/queen", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Kerrigan", value);
}

TEST_F(RimeConfigTest, Config_GetList) {
  ConfigListPtr p;
  p = config_->GetList("protoss/air_force");
  ASSERT_TRUE(p);
  ASSERT_EQ(4, p->size());
  ConfigValuePtr element;
  std::string value;
  element = p->GetValueAt(0);
  ASSERT_TRUE(element);
  ASSERT_TRUE(element->GetString(&value));
  EXPECT_EQ("scout", value);
  element = p->GetValueAt(3);
  ASSERT_TRUE(element);
  ASSERT_TRUE(element->GetString(&value));
  EXPECT_EQ("arbiter", value);

}

TEST_F(RimeConfigTest, Config_GetMap) {
  ConfigMapPtr p;
  p = config_->GetMap("terrans/tank/cost");
  ASSERT_TRUE(p);
  EXPECT_FALSE(p->HasKey("rime"));
  ASSERT_TRUE(p->HasKey("time"));
  ConfigValuePtr item;
  std::string time;
  int mineral = 0;
  int gas = 0;
  item = p->GetValue("time");
  ASSERT_TRUE(item);
  ASSERT_TRUE(item->GetString(&time));
  EXPECT_EQ("30 seconds", time);
  item = p->GetValue("mineral");
  ASSERT_TRUE(item);
  ASSERT_TRUE(item->GetInt(&mineral));
  EXPECT_EQ(150, mineral);
  item = p->GetValue("gas");
  ASSERT_TRUE(item);
  ASSERT_TRUE(item->GetInt(&gas));
  EXPECT_EQ(100, gas);
}

TEST(RimeConfigWriterTest, Greetings) {
  scoped_ptr<Config> config(new Config);
  ASSERT_TRUE(config);
  // creating contents
  EXPECT_TRUE(config->SetItem("/", ConfigItemPtr(new ConfigMap)));
  ConfigItemPtr terran_greetings(new ConfigValue("Greetings, Terrans!"));
  ConfigItemPtr zerg_greetings(new ConfigValue("Zergsss are coming!"));
  ConfigItemPtr zergs_coming(new ConfigValue(true));
  ConfigItemPtr zergs_population(new ConfigValue(1000000));
  EXPECT_TRUE(config->SetItem("greetings", terran_greetings));
  EXPECT_TRUE(config->SetItem("zergs/overmind/greetings", zerg_greetings));
  EXPECT_TRUE(config->SetItem("zergs/going", zergs_coming));
  EXPECT_TRUE(config->SetItem("zergs/statistics/population", zergs_population));
  // will not create subkeys over an existing value node
  EXPECT_FALSE(config->SetItem("zergs/going/home", zerg_greetings));
  // saving
  EXPECT_TRUE(config->SaveToFile("config_writer_test.yaml"));
  // verify
  scoped_ptr<Config> config2(new Config);
  ASSERT_TRUE(config2);
  EXPECT_TRUE(config2->LoadFromFile("config_writer_test.yaml"));
  std::string the_greetings;
  EXPECT_TRUE(config2->GetString("greetings", &the_greetings));
  EXPECT_EQ("Greetings, Terrans!", the_greetings);
  EXPECT_TRUE(config2->GetString("zergs/overmind/greetings", &the_greetings));
  EXPECT_EQ("Zergsss are coming!", the_greetings);
  bool coming = false;
  EXPECT_TRUE(config2->GetBool("zergs/going", &coming));
  EXPECT_TRUE(coming);
  int population = 0;
  EXPECT_TRUE(config2->GetInt("zergs/statistics/population", &population));
  EXPECT_EQ(1000000, population);
  EXPECT_FALSE(config2->GetString("zergs/going/home", &the_greetings));
  // modifying tree
  EXPECT_TRUE(config2->SetInt("zergs/statistics/population", population / 2));
  EXPECT_TRUE(config2->SetString("protoss/residence", "Aiur"));
  EXPECT_TRUE(config2->SetItem("zergs/overmind", ConfigItemPtr()));
  EXPECT_TRUE(config2->SaveToFile("config_rewriter_test.yaml"));
  // verify
  scoped_ptr<Config> config3(new Config);
  ASSERT_TRUE(config3);
  EXPECT_TRUE(config3->LoadFromFile("config_rewriter_test.yaml"));
  EXPECT_TRUE(config3->GetInt("zergs/statistics/population", &population));
  EXPECT_EQ(500000, population);
  std::string value;
  EXPECT_TRUE(config3->GetString("protoss/residence", &value));
  EXPECT_EQ("Aiur", value);
  // deleted
  EXPECT_FALSE(config3->GetString("zergs/overmind/greetings", &value));
  EXPECT_FALSE(config3->GetMap("zergs/overmind"));
}
