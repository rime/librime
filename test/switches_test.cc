#include <gtest/gtest.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/ticket.h>

using namespace rime;

namespace {

the<Config> SharedUserConfig() {
  return the<Config>(Config::Require("user_config")->Create("user"));
}

void ClearSavedOption(const string& name) {
  SharedUserConfig()->SetItem("var/option/" + name, nullptr);
}

}  // namespace

TEST(SwitchPersistTest, RadioDefaultsToFirstWhenUnset) {
  ClearSavedOption("style_a");
  ClearSavedOption("style_b");
  ClearSavedOption("style_c");

  the<Engine> engine(Engine::Create());

  auto config = new Config;
  (*config)["switches"][0]["options"][0] = "style_a";
  (*config)["switches"][0]["options"][1] = "style_b";
  (*config)["switches"][0]["options"][2] = "style_c";
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_TRUE(engine->context()->get_option("style_a"));
  EXPECT_FALSE(engine->context()->get_option("style_b"));
  EXPECT_FALSE(engine->context()->get_option("style_c"));
}

TEST(SwitchPersistTest, SavedValueTakesPrecedenceOverReset) {
  auto user = SharedUserConfig();
  user->SetBool("var/option/saved_off", false);

  the<Engine> engine(Engine::Create());
  // RestoreSavedOptions already applied saved_off=false at construction.
  EXPECT_FALSE(engine->context()->get_option("saved_off"));

  auto config = new Config;
  (*config)["switches"][0]["name"] = "saved_off";
  (*config)["switches"][0]["reset"] = 1;
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  // Saved value must not be overwritten by reset.
  EXPECT_FALSE(engine->context()->get_option("saved_off"));

  ClearSavedOption("saved_off");
}

TEST(SwitchPersistTest, ResetAppliesWhenNoSavedValue) {
  ClearSavedOption("reset_on");

  the<Engine> engine(Engine::Create());

  auto config = new Config;
  (*config)["switches"][0]["name"] = "reset_on";
  (*config)["switches"][0]["reset"] = 1;
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_TRUE(engine->context()->get_option("reset_on"));
}

TEST(SwitchPersistTest, SavedRadioNotOverriddenByDefaultFirst) {
  auto user = SharedUserConfig();
  user->SetBool("var/option/lang_a", false);
  user->SetBool("var/option/lang_b", true);

  the<Engine> engine(Engine::Create());

  auto config = new Config;
  (*config)["switches"][0]["options"][0] = "lang_a";
  (*config)["switches"][0]["options"][1] = "lang_b";
  auto schema = new Schema;
  schema->set_config(config);

  engine->ApplySchema(schema);

  EXPECT_FALSE(engine->context()->get_option("lang_a"));
  EXPECT_TRUE(engine->context()->get_option("lang_b"));

  ClearSavedOption("lang_a");
  ClearSavedOption("lang_b");
}

TEST(SwitchPersistTest, IsAutoSaveAlwaysTrue) {
  the<Engine> engine(Engine::Create());
  Switcher switcher(Ticket(engine.get()));
  EXPECT_TRUE(switcher.IsAutoSave("full_shape"));
  EXPECT_TRUE(switcher.IsAutoSave("any_custom_option"));
  EXPECT_TRUE(switcher.IsAutoSave("not_in_legacy_save_options"));
}

TEST(SwitchPersistTest, RestoreAllSavedOptions) {
  ClearSavedOption("restore_a");
  ClearSavedOption("restore_b");

  auto user = SharedUserConfig();
  user->SetBool("var/option/restore_a", true);
  user->SetBool("var/option/restore_b", false);

  // New engine restores all var/option/* once at construction.
  the<Engine> engine(Engine::Create());

  EXPECT_TRUE(engine->context()->get_option("restore_a"));
  EXPECT_FALSE(engine->context()->get_option("restore_b"));
  EXPECT_FALSE(engine->context()->get_option("restore_missing"));

  ClearSavedOption("restore_a");
  ClearSavedOption("restore_b");
}
