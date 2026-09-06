#include <gtest/gtest.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>

using namespace rime;

TEST(SwitchDefaultTest, AppliesDefaultWhenOptionIsUnset) {
  the<Engine> engine(Engine::Create());

  auto config = new Config;
  (*config)["switches"][0]["name"] = "default_on";
  (*config)["switches"][0]["default"] = 1;
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_TRUE(engine->context()->get_option("default_on"));
}

TEST(SwitchDefaultTest, PreservesExistingOption) {
  the<Engine> engine(Engine::Create());
  engine->context()->set_option("saved_off", false);

  auto config = new Config;
  (*config)["switches"][0]["name"] = "saved_off";
  (*config)["switches"][0]["default"] = 1;
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_FALSE(engine->context()->get_option("saved_off"));
}

TEST(SwitchDefaultTest, ResetTakesPrecedenceOverDefault) {
  the<Engine> engine(Engine::Create());
  engine->context()->set_option("reset_on", false);

  auto config = new Config;
  (*config)["switches"][0]["name"] = "reset_on";
  (*config)["switches"][0]["default"] = 0;
  (*config)["switches"][0]["reset"] = 1;
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_TRUE(engine->context()->get_option("reset_on"));
}
