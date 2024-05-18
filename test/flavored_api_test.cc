#include <gtest/gtest.h>
#include <rime_api_stdbool.h>
#include <rime_api.h>
#include <rime_levers_api.h>

TEST(RimeApiStdboolTest, GetContext) {
  RIME_FLAVORED(RimeApi)* rime = RIME_FLAVORED(rime_get_api)();
  ASSERT_TRUE(bool(rime));

  ASSERT_TRUE(RIME_API_AVAILABLE(rime, create_session));
  RimeSessionId test_session = rime->create_session();
  ASSERT_NE(0, test_session);

  ASSERT_TRUE(RIME_API_AVAILABLE(rime, get_context));
  RIME_STRUCT(RIME_FLAVORED(RimeContext), ctx);
  ASSERT_TRUE(rime->get_context(test_session, &ctx));
  ASSERT_EQ(0, ctx.menu.num_candidates);

  ASSERT_TRUE(RIME_API_AVAILABLE(rime, get_status));
  RIME_STRUCT(RIME_FLAVORED(RimeStatus), status);
  ASSERT_TRUE(rime->get_status(test_session, &status));
  ASSERT_FALSE(status.is_composing);
}

TEST(RimeLeversApiStdboolTest, CustomSettings) {
  RIME_FLAVORED(RimeApi)* rime = RIME_FLAVORED(rime_get_api)();
  RimeModule* module = rime->find_module("levers_stdbool");
  ASSERT_TRUE(bool(module));
  RIME_FLAVORED(RimeLeversApi)* levers =
      (RIME_FLAVORED(RimeLeversApi)*)module->get_api();
  ASSERT_TRUE(bool(levers));

  ASSERT_TRUE(RIME_API_AVAILABLE(levers, custom_settings_init));
  RimeCustomSettings* custom_settings =
      levers->custom_settings_init("flavored_api_test", "rime_test");
  ASSERT_TRUE(bool(custom_settings));

  ASSERT_TRUE(RIME_API_AVAILABLE(levers, customize_bool));
  ASSERT_TRUE(levers->customize_bool(custom_settings, "test_key", true));

  levers->custom_settings_destroy(custom_settings);
}
