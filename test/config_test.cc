//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-06 Zou xu <zouivex@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>

using namespace rime;

class RimeConfigTest : public ::testing::Test {
 protected:
  RimeConfigTest() = default;

  virtual void SetUp() {
    component_.reset(new ConfigComponent<ConfigLoader>);
    config_.reset(component_->Create("config_test"));
  }

  virtual void TearDown() {
  }

  the<Config::Component> component_;
  the<Config> config_;
};

TEST(RimeConfigComponentTest, RoundTrip) {
  // registration
  Registry& r = Registry::instance();
  r.Register(
      "test_config",
      new ConfigComponent<ConfigLoader, UserConfigResourceProvider>(
          [](ConfigLoader* loader) {
            loader->set_auto_save(true);
          }));
  // find component
  Config::Component* cc = Config::Require("test_config");
  ASSERT_TRUE(cc != NULL);
  // create config and write modifications to file
  {
    the<Config> config(cc->Create("config_round_trip_test"));
    EXPECT_TRUE(bool(config));
    EXPECT_TRUE(config->SetString("key", "value"));
  }
  // read from file and verify contents
  {
    the<Config> config(cc->Create("config_round_trip_test"));
    EXPECT_TRUE(bool(config));
    string value;
    EXPECT_TRUE(config->GetString("key", &value));
    EXPECT_EQ("value", value);
  }
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
  string value;
  ret = config_->GetString("protoss/residence", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Aiur", value);

  ret = config_->GetString("zerg/queen", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Kerrigan", value);
}

TEST_F(RimeConfigTest, Config_GetList) {
  an<ConfigList> p;
  p = config_->GetList("protoss/air_force");
  ASSERT_TRUE(bool(p));
  ASSERT_EQ(4, p->size());
  an<ConfigValue> element;
  string value;
  element = p->GetValueAt(0);
  ASSERT_TRUE(bool(element));
  ASSERT_TRUE(element->GetString(&value));
  EXPECT_EQ("scout", value);
  element = p->GetValueAt(3);
  ASSERT_TRUE(bool(element));
  ASSERT_TRUE(element->GetString(&value));
  EXPECT_EQ("arbiter", value);

}

TEST_F(RimeConfigTest, Config_GetMap) {
  an<ConfigMap> p;
  p = config_->GetMap("terrans/tank/cost");
  ASSERT_TRUE(bool(p));
  EXPECT_FALSE(p->HasKey("rime"));
  ASSERT_TRUE(p->HasKey("time"));
  an<ConfigValue> item;
  string time;
  int mineral = 0;
  int gas = 0;
  item = p->GetValue("time");
  ASSERT_TRUE(bool(item));
  ASSERT_TRUE(item->GetString(&time));
  EXPECT_EQ("30 seconds", time);
  item = p->GetValue("mineral");
  ASSERT_TRUE(bool(item));
  ASSERT_TRUE(item->GetInt(&mineral));
  EXPECT_EQ(150, mineral);
  item = p->GetValue("gas");
  ASSERT_TRUE(bool(item));
  ASSERT_TRUE(item->GetInt(&gas));
  EXPECT_EQ(100, gas);
}

TEST(RimeConfigWriterTest, Greetings) {
  the<Config> config(new Config);
  ASSERT_TRUE(bool(config));
  // creating contents
  EXPECT_TRUE(config->SetItem("/", New<ConfigMap>()));
  an<ConfigItem> terran_greetings = New<ConfigValue>("Greetings, Terrans!");
  an<ConfigItem> zerg_greetings = New<ConfigValue>("Zergsss are coming!");
  an<ConfigItem> zergs_coming = New<ConfigValue>(true);
  an<ConfigItem> zergs_population = New<ConfigValue>(1000000);
  EXPECT_TRUE(config->SetItem("greetings", terran_greetings));
  EXPECT_TRUE(config->SetItem("zergs/overmind/greetings", zerg_greetings));
  EXPECT_TRUE(config->SetItem("zergs/going", zergs_coming));
  EXPECT_TRUE(config->SetItem("zergs/statistics/population", zergs_population));
  // will not create subkeys over an existing value node
  EXPECT_FALSE(config->SetItem("zergs/going/home", zerg_greetings));
  // saving
  EXPECT_TRUE(config->SaveToFile("config_writer_test.yaml"));
  // verify
  the<Config> config2(new Config);
  ASSERT_TRUE(bool(config2));
  EXPECT_TRUE(config2->LoadFromFile("config_writer_test.yaml"));
  string the_greetings;
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
  EXPECT_TRUE(config2->SetItem("zergs/overmind", nullptr));
  EXPECT_TRUE(config2->SaveToFile("config_rewriter_test.yaml"));
  // verify
  the<Config> config3(new Config);
  ASSERT_TRUE(bool(config3));
  EXPECT_TRUE(config3->LoadFromFile("config_rewriter_test.yaml"));
  EXPECT_TRUE(config3->GetInt("zergs/statistics/population", &population));
  EXPECT_EQ(500000, population);
  string value;
  EXPECT_TRUE(config3->GetString("protoss/residence", &value));
  EXPECT_EQ("Aiur", value);
  // deleted
  EXPECT_FALSE(config3->GetString("zergs/overmind/greetings", &value));
  EXPECT_FALSE(config3->GetMap("zergs/overmind"));
}

TEST(RimeConfigxxTest, Operations) {
  Config config;
  config["str"] = "STR";
  config["nested"]["bool"] = true;
  config["nested"]["int"] = 1000000;
  config["nested"]["greetings"] = New<ConfigValue>("Hello!");
  config["list"][0] = New<ConfigMap>();
  config["list"][0]["abc"] = "ABC";
  config["list"].Append(New<ConfigMap>());
  config["list"][1]["abc"] = "ZYX";
  config["list"][2] = config["nested"]["greetings"];
  config["list"][3]["abc"] = "123";
  config["str"].Clear();
  EXPECT_TRUE(config["str"].IsNull());
  EXPECT_EQ(true, config["nested"]["bool"].ToBool());
  EXPECT_EQ(1000000, config["nested"]["int"].ToInt());
  EXPECT_EQ("Hello!", config["nested"]["greetings"].ToString());
  EXPECT_TRUE(config["list"].IsList());
  EXPECT_EQ(4, config["list"].size());
  EXPECT_EQ("123", config["list"][3]["abc"].ToString());
  EXPECT_EQ("Hello!", config["list"][2].ToString());
  an<ConfigItem> v1(config["list"][2]);
  an<ConfigItem> v2(config["nested"]["greetings"]);
  EXPECT_EQ(v1, v2);
  EXPECT_TRUE(config.modified());
  EXPECT_TRUE(config.SaveToFile("rime_configxx_test.yaml"));
  EXPECT_TRUE(config.LoadFromFile("rime_configxx_test.yaml"));
  EXPECT_TRUE(config["str"].IsNull());
  EXPECT_FALSE(config.modified());
  EXPECT_EQ("Hello!", config["nested"]["greetings"].ToString());
  config["list"].Append(New<ConfigValue>("orz"));
  EXPECT_TRUE(config.modified());
  ConfigMapEntryRef r(config["nested"]["greetings"]);
  EXPECT_EQ("Hello!", r.ToString());
}

TEST(RimeConfigListKeyPathTest, Greetings) {
  int id = 0;
  int value = 0;
  the<Config> config(new Config);
  ASSERT_TRUE(bool(config));
  // append items
  EXPECT_TRUE(config->SetInt("list/@next/id", 1));
  EXPECT_TRUE(config->SetInt("list/@last/value", 100));
  EXPECT_TRUE(config->SetInt("list/@next/id", 2));
  EXPECT_TRUE(config->SetInt("list/@last/value", 200));
  ASSERT_TRUE(bool(config->GetList("list")));
  ASSERT_EQ(2, config->GetList("list")->size());
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@0/value", &value));
  EXPECT_EQ(100, value);
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@1/value", &value));
  EXPECT_EQ(200, value);
  // insert items
  EXPECT_TRUE(config->SetInt("list/@before 0/id", 3));
  EXPECT_TRUE(config->SetInt("list/@0/value", 50));
  EXPECT_TRUE(config->SetInt("list/@after last/id", 4));
  EXPECT_TRUE(config->SetInt("list/@last/value", 400));
  ASSERT_EQ(4, config->GetList("list")->size());
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@0/value", &value));
  EXPECT_EQ(50, value);
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@1/value", &value));
  EXPECT_EQ(100, value);
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@2/value", &value));
  EXPECT_EQ(200, value);
  value = 0;
  EXPECT_TRUE(config->GetInt("list/@3/value", &value));
  EXPECT_EQ(400, value);
  // refer to last item in an empty list
  config->GetList("list")->Clear();
  ASSERT_EQ(0, config->GetList("list")->size());
  config->SetInt("list/@after last/id", 5);
  EXPECT_EQ(1, config->GetList("list")->size());
  id = 0;
  EXPECT_TRUE(config->GetInt("list/@last/id", &id));
  EXPECT_EQ(5, id);
  config->GetList("list")->Clear();
  ASSERT_EQ(0, config->GetList("list")->size());
  config->SetInt("list/@before last/id", 6);
  EXPECT_EQ(1, config->GetList("list")->size());
  id = 0;
  EXPECT_TRUE(config->GetInt("list/@last/id", &id));
  EXPECT_EQ(6, id);
}
