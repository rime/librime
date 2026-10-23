#include <gtest/gtest.h>

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rime/common.h>
#include <rime/setup.h>

namespace {

struct CapturedRecord {
  int level = 0;
  std::string message;
  std::string file;
  int line = 0;
  std::string function;
};

struct Capture {
  std::mutex mutex;
  std::vector<CapturedRecord> records;
};

void CaptureHandler(void* context, const RimeLogRecord* record) {
  auto* capture = static_cast<Capture*>(context);
  std::lock_guard<std::mutex> lock(capture->mutex);
  capture->records.push_back({record->level,
                              record->message ? record->message : "",
                              record->file ? record->file : "", record->line,
                              record->function ? record->function : ""});
}

class LoggingFacadeTest : public ::testing::Test {
 protected:
  void TearDown() override { rime::SetLogHandler(nullptr, nullptr); }
};

TEST_F(LoggingFacadeTest, DiscardsRecordsWithoutHandler) {
  rime::SetLogHandler(nullptr, nullptr);

  LOG(INFO) << "discarded";
  LOG(WARNING) << "discarded";
  LOG(ERROR) << "discarded";
  LOG_IF(INFO, false) << "discarded";
  LOG_EVERY_N(INFO, 10) << "discarded";
  VLOG(1) << "discarded";

  SUCCEED();
}

TEST_F(LoggingFacadeTest, CapturesRecordMetadata) {
  Capture capture;
  rime::SetLogHandler(&CaptureHandler, &capture);

  LOG(INFO) << "hello " << 42;

  ASSERT_EQ(capture.records.size(), 1);
  const auto& record = capture.records[0];
  EXPECT_EQ(record.level, RIME_LOG_INFO);
  EXPECT_EQ(record.message, "hello 42");
  EXPECT_NE(record.file.find("logging_facade_test.cc"), std::string::npos);
  EXPECT_GT(record.line, 0);
  EXPECT_FALSE(record.function.empty());
}

TEST_F(LoggingFacadeTest, HonorsLogCondition) {
  Capture capture;
  rime::SetLogHandler(&CaptureHandler, &capture);

  LOG_IF(ERROR, false) << "not logged";
  LOG_IF(ERROR, true) << "logged";

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].level, RIME_LOG_ERROR);
  EXPECT_EQ(capture.records[0].message, "logged");
}

TEST_F(LoggingFacadeTest, ClearsHandler) {
  Capture capture;
  rime::SetLogHandler(&CaptureHandler, &capture);

  LOG(INFO) << "logged";
  rime::SetLogHandler(nullptr, nullptr);
  LOG(INFO) << "not logged";

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].message, "logged");
}

TEST_F(LoggingFacadeTest, InstallsHandlerThroughCApi) {
  RimeApi* api = rime_get_api();
  ASSERT_NE(api, nullptr);
  ASSERT_TRUE(RIME_API_AVAILABLE(api, set_log_handler));

  Capture capture;
  api->set_log_handler(&CaptureHandler, &capture);
  LOG(INFO) << "via c api";
  api->set_log_handler(nullptr, nullptr);

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].message, "via c api");
}

TEST_F(LoggingFacadeTest, SupportsConcurrentLogging) {
  Capture capture;
  rime::SetLogHandler(&CaptureHandler, &capture);

  constexpr int kThreads = 4;
  constexpr int kRecordsPerThread = 100;
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([kRecordsPerThread] {
      for (int j = 0; j < kRecordsPerThread; ++j) {
        LOG(INFO) << "threaded";
      }
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(capture.records.size(), kThreads * kRecordsPerThread);
}

TEST_F(LoggingFacadeTest, CheckMacrosCompileAndPass) {
  int value = 7;
  CHECK(true);
  CHECK_EQ(value, 7);
  CHECK_NE(value, 8);
  CHECK_LE(value, 7);
  CHECK_LT(value, 8);
  CHECK_GE(value, 7);
  CHECK_GT(value, 6);
  CHECK_STREQ("rime", "rime");
  CHECK_STRNE("rime", "librime");
  CHECK_STRCASEEQ("Rime", "rime");
  CHECK_STRCASENE("Rime", "librime");
  EXPECT_EQ(CHECK_NOTNULL(&value), &value);
}

#if GTEST_HAS_DEATH_TEST
TEST_F(LoggingFacadeTest, CheckFailureAborts) {
  rime::SetLogHandler(nullptr, nullptr);
  EXPECT_DEATH({ CHECK(false); }, "Check failed");
}
#endif

}  // namespace
