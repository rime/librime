#include <gtest/gtest.h>
#include <rime_api.h>

#include <rime/last_error.h>

namespace {

const char* kNoSuchConfig = "no_such_config_12345";
const char* kNoSuchSchema = "no_such_schema_12345";
const char* kNoSuchModule = "no_such_module_12345";

class RimeLastErrorTest : public ::testing::Test {
 protected:
  void SetUp() override { rime::ClearLastError(); }
  void TearDown() override { rime::ClearLastError(); }

  RimeApi* rime = rime_get_api();
};

TEST_F(RimeLastErrorTest, NoErrorInitially) {
  EXPECT_EQ(RIME_ERROR_NONE, RimeGetLastError());
  EXPECT_EQ(RIME_ERROR_NONE, rime->get_last_error());
}

TEST_F(RimeLastErrorTest, ReportsInvalidArgument) {
  ASSERT_TRUE(RIME_API_AVAILABLE(rime, config_open));
  RimeConfig config = {0};
  EXPECT_EQ(False, rime->config_open(nullptr, &config));
  EXPECT_EQ(RIME_ERROR_INVALID_ARGUMENT, rime->get_last_error());

  EXPECT_EQ(False, rime->config_open("config_test", nullptr));
  EXPECT_EQ(RIME_ERROR_INVALID_ARGUMENT, rime->get_last_error());

  ASSERT_TRUE(RIME_API_AVAILABLE(rime, find_module));
  EXPECT_EQ(nullptr, rime->find_module(nullptr));
  EXPECT_EQ(RIME_ERROR_INVALID_ARGUMENT, rime->get_last_error());
}

TEST_F(RimeLastErrorTest, ReportsSessionNotFound) {
  ASSERT_TRUE(RIME_API_AVAILABLE(rime, create_session));
  RimeSessionId session_id = rime->create_session();
  ASSERT_NE(0, session_id);
  rime->destroy_session(session_id);

  RIME_STRUCT(RimeStatus, status);
  EXPECT_EQ(False, rime->get_status(session_id, &status));
  EXPECT_EQ(RIME_ERROR_SESSION_NOT_FOUND, rime->get_last_error());
}

TEST_F(RimeLastErrorTest, ReportsConfigNotFound) {
  RimeConfig config = {0};
  EXPECT_EQ(False, rime->config_open(kNoSuchConfig, &config));
  EXPECT_EQ(RIME_ERROR_CONFIG_NOT_FOUND, rime->get_last_error());

  // a schema is a config file of the `schema` component
  EXPECT_EQ(False, rime->schema_open(kNoSuchSchema, &config));
  EXPECT_EQ(RIME_ERROR_SCHEMA_NOT_FOUND, rime->get_last_error());

  RimeSessionId session_id = rime->create_session();
  ASSERT_NE(0, session_id);
  EXPECT_EQ(False, rime->select_schema(session_id, kNoSuchSchema));
  EXPECT_EQ(RIME_ERROR_SCHEMA_NOT_FOUND, rime->get_last_error());
  rime->destroy_session(session_id);
}

TEST_F(RimeLastErrorTest, FailedOpenLeavesClosedConfig) {
  RimeConfig config = {0};
  EXPECT_EQ(False, rime->config_open(kNoSuchConfig, &config));
  ASSERT_EQ(RIME_ERROR_CONFIG_NOT_FOUND, rime->get_last_error());
  // A failed open must not leave a dangling config behind.
  EXPECT_EQ(nullptr, config.ptr);

  // Accessors must not crash on such a config, nor hide the error.
  Bool bool_value = False;
  int int_value = 0;
  double double_value = 0.0;
  EXPECT_EQ(False, rime->config_get_bool(&config, "some/key", &bool_value));
  EXPECT_EQ(False, rime->config_get_int(&config, "some/key", &int_value));
  EXPECT_EQ(False, rime->config_get_double(&config, "some/key", &double_value));
  EXPECT_EQ(RIME_ERROR_CONFIG_NOT_FOUND, rime->get_last_error());

  // Closing the failed config is a no-op rather than a newly reported error.
  EXPECT_EQ(False, rime->config_close(&config));
  EXPECT_EQ(RIME_ERROR_CONFIG_NOT_FOUND, rime->get_last_error());
}

TEST_F(RimeLastErrorTest, ReportsInvalidConfigString) {
  ASSERT_TRUE(RIME_API_AVAILABLE(rime, config_load_string));
  RimeConfig config = {0};
  EXPECT_EQ(False, rime->config_load_string(&config, "not: [valid"));
  EXPECT_EQ(RIME_ERROR_CONFIG_INVALID, rime->get_last_error());

  EXPECT_EQ(False, rime->config_load_string(&config, nullptr));
  EXPECT_EQ(RIME_ERROR_INVALID_ARGUMENT, rime->get_last_error());
  rime->config_close(&config);
}

TEST_F(RimeLastErrorTest, ReportsModuleNotFound) {
  EXPECT_EQ(nullptr, rime->find_module(kNoSuchModule));
  EXPECT_EQ(RIME_ERROR_MODULE_NOT_FOUND, rime->get_last_error());
}

TEST_F(RimeLastErrorTest, SuccessfulCallClearsError) {
  EXPECT_EQ(nullptr, rime->find_module(kNoSuchModule));
  ASSERT_EQ(RIME_ERROR_MODULE_NOT_FOUND, rime->get_last_error());

  // any successful call forgets the error of the previous call
  RimeSessionId session_id = rime->create_session();
  ASSERT_NE(0, session_id);
  RIME_STRUCT(RimeStatus, status);
  EXPECT_EQ(True, rime->get_status(session_id, &status));
  EXPECT_EQ(RIME_ERROR_NONE, rime->get_last_error());

  RimeConfig config = {0};
  EXPECT_EQ(True, rime->config_open("config_test", &config));
  EXPECT_EQ(RIME_ERROR_NONE, rime->get_last_error());
  EXPECT_EQ(True, rime->config_close(&config));

  rime->destroy_session(session_id);
}

}  // namespace
