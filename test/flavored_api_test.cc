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
