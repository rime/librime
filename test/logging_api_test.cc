#include <gtest/gtest.h>

#include <rime/build_config.h>

#include <rime_api.h>

namespace {

void TestLogHandler(void* /*context_object*/, const RimeLogRecord* /*record*/) {
}

}  // namespace

TEST(LoggingApiTest, ReportsHandlerSupport) {
  RimeApi* api = rime_get_api();
  ASSERT_NE(api, nullptr);
  ASSERT_TRUE(RIME_API_AVAILABLE(api, set_log_handler));

#ifdef RIME_ENABLE_LOGGING
  EXPECT_FALSE(api->set_log_handler(&TestLogHandler, nullptr));
  EXPECT_FALSE(api->set_log_handler(nullptr, nullptr));
#else
  EXPECT_TRUE(api->set_log_handler(&TestLogHandler, nullptr));
  EXPECT_TRUE(api->set_log_handler(nullptr, nullptr));
#endif
}
