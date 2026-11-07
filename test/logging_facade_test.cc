#include <rime/build_config.h>

#ifndef RIME_ENABLE_LOGGING

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
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
};

struct Capture {
  std::mutex mutex;
  std::vector<CapturedRecord> records;
};

void CaptureRecord(Capture* capture, const RimeLogRecord& record) {
  std::lock_guard<std::mutex> lock(capture->mutex);
  capture->records.push_back({record.level,
                              record.message ? record.message : "",
                              record.file ? record.file : "", record.line});
}

void CaptureHandler(void* context, const RimeLogRecord* record) {
  CaptureRecord(static_cast<Capture*>(context), *record);
}

rime::LogHandler CaptureInto(Capture* capture) {
  return [capture](const RimeLogRecord& record) {
    CaptureRecord(capture, record);
  };
}

class LoggingFacadeTest : public ::testing::Test {
 protected:
  void TearDown() override { rime::ClearLogHandler(); }
};

TEST_F(LoggingFacadeTest, DiscardsRecordsWithoutHandler) {
  rime::ClearLogHandler();

  LOG(INFO) << "discarded";
  LOG(WARNING) << "discarded";
  LOG(ERROR) << "discarded";
  LOG_IF(INFO, false) << "discarded";

  SUCCEED();
}

TEST_F(LoggingFacadeTest, DoesNotEvaluateArgumentsWhenDisabled) {
  rime::ClearLogHandler();

  int calls = 0;
  auto side_effect = [&calls] {
    ++calls;
    return "discarded";
  };

  LOG(INFO) << side_effect();
  LOG_IF(INFO, false) << side_effect();
  DLOG(INFO) << side_effect();
  CHECK(true) << side_effect();
  DCHECK(true) << side_effect();
  EXPECT_EQ(calls, 0);

  Capture capture;
  rime::SetLogHandler(CaptureInto(&capture));
  LOG(INFO) << side_effect();

  EXPECT_EQ(calls, 1);
  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].message, "discarded");
}

TEST_F(LoggingFacadeTest, CapturesRecordMetadata) {
  Capture capture;
  rime::SetLogHandler(CaptureInto(&capture));

  LOG(INFO) << "hello " << 42;

  ASSERT_EQ(capture.records.size(), 1);
  const auto& record = capture.records[0];
  EXPECT_EQ(record.level, RIME_LOG_INFO);
  EXPECT_EQ(record.message, "hello 42");
  EXPECT_NE(record.file.find("logging_facade_test.cc"), std::string::npos);
  EXPECT_GT(record.line, 0);
}

TEST_F(LoggingFacadeTest, HonorsLogCondition) {
  Capture capture;
  rime::SetLogHandler(CaptureInto(&capture));

  LOG_IF(ERROR, false) << "not logged";
  LOG_IF(ERROR, true) << "logged";

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].level, RIME_LOG_ERROR);
  EXPECT_EQ(capture.records[0].message, "logged");
}

TEST_F(LoggingFacadeTest, ClearsHandler) {
  Capture capture;
  rime::SetLogHandler(CaptureInto(&capture));

  LOG(INFO) << "logged";
  rime::ClearLogHandler();
  LOG(INFO) << "not logged";

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].message, "logged");
}

TEST_F(LoggingFacadeTest, InstallsHandlerThroughCApi) {
  RimeApi* api = rime_get_api();
  ASSERT_NE(api, nullptr);
  ASSERT_TRUE(RIME_API_AVAILABLE(api, set_log_handler));

  Capture capture;
  EXPECT_TRUE(api->set_log_handler(&CaptureHandler, &capture));
  LOG(INFO) << "via c api";
  EXPECT_TRUE(api->set_log_handler(nullptr, nullptr));

  ASSERT_EQ(capture.records.size(), 1);
  EXPECT_EQ(capture.records[0].message, "via c api");
}

TEST_F(LoggingFacadeTest, ClearWaitsForInFlightCallbacks) {
  std::promise<void> entered;
  std::promise<void> release;
  std::atomic<bool> callback_finished{false};

  rime::SetLogHandler([&](const RimeLogRecord&) {
    entered.set_value();
    release.get_future().wait();
    callback_finished = true;
  });

  std::thread logger([] { LOG(INFO) << "blocking"; });
  entered.get_future().wait();

  std::promise<void> cleared;
  std::thread clearer([&] {
    rime::ClearLogHandler();
    cleared.set_value();
  });

  EXPECT_EQ(cleared.get_future().wait_for(std::chrono::milliseconds(50)),
            std::future_status::timeout);
  EXPECT_FALSE(callback_finished.load());

  release.set_value();
  clearer.join();
  logger.join();
  EXPECT_TRUE(callback_finished.load());
}

TEST_F(LoggingFacadeTest, SupportsConcurrentLogging) {
  Capture capture;
  rime::SetLogHandler(CaptureInto(&capture));

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
  CHECK(value == 7);
  CHECK(true) << "streamed";
  DCHECK(value == 7);

  // The statement-shaped macros must remain valid as unbraced if/else bodies.
  if (value == 7)
    LOG(INFO) << "if";
  else
    LOG(INFO) << "else";
}

#if GTEST_HAS_DEATH_TEST
TEST_F(LoggingFacadeTest, CheckFailureAborts) {
  rime::ClearLogHandler();
  EXPECT_DEATH({ CHECK(false); }, "Check failed");
}
#endif

}  // namespace

#endif  // RIME_ENABLE_LOGGING
