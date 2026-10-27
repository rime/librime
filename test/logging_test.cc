//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/build_config.h>

#ifdef RIME_ENABLE_LOGGING

#include <gtest/gtest.h>

#include <rime/logging.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <process.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

int ProcessId() {
#ifdef _WIN32
  return _getpid();
#else
  return getpid();
#endif
}

#ifdef _WIN32
class ScopedWin32Handle {
 public:
  explicit ScopedWin32Handle(HANDLE handle) : handle_(handle) {}
  ~ScopedWin32Handle() {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE)
      ::CloseHandle(handle_);
  }

  ScopedWin32Handle(const ScopedWin32Handle&) = delete;
  ScopedWin32Handle& operator=(const ScopedWin32Handle&) = delete;

  HANDLE get() const { return handle_; }

 private:
  HANDLE handle_;
};

class ScopedStderrHandle {
 public:
  explicit ScopedStderrHandle(HANDLE replacement)
      : previous_(::GetStdHandle(STD_ERROR_HANDLE)),
        replaced_(::SetStdHandle(STD_ERROR_HANDLE, replacement) != 0) {}
  ~ScopedStderrHandle() {
    if (replaced_)
      ::SetStdHandle(STD_ERROR_HANDLE, previous_);
  }

  ScopedStderrHandle(const ScopedStderrHandle&) = delete;
  ScopedStderrHandle& operator=(const ScopedStderrHandle&) = delete;

  bool replaced() const { return replaced_; }

 private:
  HANDLE previous_;
  bool replaced_;
};
#endif

class LoggingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rime::ShutdownLogging();
    log_dir_ = fs::temp_directory_path() /
               ("rime-logging-test-" + std::to_string(ProcessId()));
    std::error_code ec;
    fs::remove_all(log_dir_, ec);
    rime::InitializeLogging("rime.logging.test", 0, log_dir_.string().c_str());
    ASSERT_TRUE(rime::IsLoggingInitialized());
    ASSERT_EQ(1u, rime::GetLoggingDirectories().size());
  }

  void TearDown() override {
    rime::ShutdownLogging();
    // Restore the state set up by rime_test_main.cc's global environment.
    rime::InitializeLogging("rime.test", 0, nullptr);
    std::error_code ec;
    fs::remove_all(log_dir_, ec);
  }

  std::vector<fs::path> LogFiles() const {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(log_dir_)) {
      if (entry.is_regular_file() && !entry.is_symlink() &&
          entry.path().extension().string() == ".log") {
        files.push_back(entry.path());
      }
    }
    return files;
  }

  fs::path LogFileFor(const std::string& severity) const {
    const std::string suffix = "." + severity + ".log";
    for (const auto& entry : fs::directory_iterator(log_dir_)) {
      if (!entry.is_regular_file() || entry.is_symlink())
        continue;
      const std::string name = entry.path().filename().string();
      if (name.size() > suffix.size() &&
          name.compare(name.size() - suffix.size(), suffix.size(), suffix) ==
              0) {
        return entry.path();
      }
    }
    return {};
  }

  static std::string ReadFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
  }

  fs::path log_dir_;
};

TEST_F(LoggingTest, WritesSeverityThresholdFilesAndLinks) {
  LOG(INFO) << "info-message";
  LOG(WARNING) << "warning-message";
  LOG(ERROR) << "error-message";
  rime::ShutdownLogging();

  const auto files = LogFiles();
  ASSERT_EQ(4u, files.size())
      << "expected INFO, WARNING, ERROR and FATAL log files";

  const auto info_file = LogFileFor("INFO");
  const auto warning_file = LogFileFor("WARNING");
  const auto error_file = LogFileFor("ERROR");
  const auto fatal_file = LogFileFor("FATAL");
  ASSERT_FALSE(info_file.empty());
  ASSERT_FALSE(warning_file.empty());
  ASSERT_FALSE(error_file.empty());
  ASSERT_FALSE(fatal_file.empty());

  const std::string info = ReadFile(info_file);
  const std::string warning = ReadFile(warning_file);
  const std::string error = ReadFile(error_file);
  const std::string fatal = ReadFile(fatal_file);

  // INFO receives every message.
  EXPECT_NE(std::string::npos, info.find("info-message"));
  EXPECT_NE(std::string::npos, info.find("warning-message"));
  EXPECT_NE(std::string::npos, info.find("error-message"));
  // WARNING receives WARNING and above.
  EXPECT_EQ(std::string::npos, warning.find("info-message"));
  EXPECT_NE(std::string::npos, warning.find("warning-message"));
  EXPECT_NE(std::string::npos, warning.find("error-message"));
  // ERROR receives ERROR and above.
  EXPECT_EQ(std::string::npos, error.find("info-message"));
  EXPECT_EQ(std::string::npos, error.find("warning-message"));
  EXPECT_NE(std::string::npos, error.find("error-message"));
  // FATAL receives only FATAL messages.
  EXPECT_EQ(std::string::npos, fatal.find("info-message"));
  EXPECT_EQ(std::string::npos, fatal.find("warning-message"));
  EXPECT_EQ(std::string::npos, fatal.find("error-message"));

  // spdlog's default full pattern:
  // [YYYY-MM-DD HH:MM:SS.mmm] [logger] [level] [source:line] message
  ASSERT_FALSE(info.empty());
  ASSERT_FALSE(warning.empty());
  ASSERT_FALSE(error.empty());
  EXPECT_EQ('[', info[0]);
  EXPECT_NE(std::string::npos,
            info.find("[rime.logging.test] [info] [logging_test.cc:"));
  EXPECT_NE(std::string::npos,
            warning.find("[rime.logging.test] [warning] [logging_test.cc:"));
  EXPECT_NE(std::string::npos,
            error.find("[rime.logging.test] [error] [logging_test.cc:"));
  EXPECT_NE(std::string::npos, info.find("] info-message"));

  for (const auto& file : files) {
    const std::string name = file.filename().string();
    EXPECT_EQ(0u, name.find("rime.logging.test."));
    EXPECT_EQ(".log", name.substr(name.size() - 4));
  }

#ifndef _WIN32
  for (const auto& file : files) {
    struct stat st = {};
    ASSERT_EQ(0, ::stat(file.c_str(), &st));
    EXPECT_EQ(0600, static_cast<int>(st.st_mode & 0777));
  }
#endif

  for (const char* severity : {"INFO", "WARNING", "ERROR", "FATAL"}) {
    const fs::path link =
        log_dir_ / ("rime.logging.test." + std::string(severity));
    ASSERT_TRUE(fs::exists(link));
    if (fs::is_symlink(link)) {
      EXPECT_EQ(LogFileFor(severity).filename(), fs::read_symlink(link));
    }
  }
}

TEST_F(LoggingTest, FiltersSeveritiesBelowMinLevel) {
  rime::ShutdownLogging();
  rime::InitializeLogging("rime.logging.test", 2, log_dir_.string().c_str());
  LOG(INFO) << "filtered-info";
  LOG(WARNING) << "filtered-warning";
  LOG(ERROR) << "kept-error";
  rime::ShutdownLogging();

  for (const char* severity : {"INFO", "WARNING", "ERROR", "FATAL"}) {
    const auto file = LogFileFor(severity);
    ASSERT_FALSE(file.empty()) << severity;
    const std::string content = ReadFile(file);
    EXPECT_EQ(std::string::npos, content.find("filtered-info"));
    EXPECT_EQ(std::string::npos, content.find("filtered-warning"));
  }

  EXPECT_NE(std::string::npos, ReadFile(LogFileFor("INFO")).find("kept-error"));
  EXPECT_NE(std::string::npos,
            ReadFile(LogFileFor("WARNING")).find("kept-error"));
  EXPECT_NE(std::string::npos,
            ReadFile(LogFileFor("ERROR")).find("kept-error"));
  EXPECT_EQ(std::string::npos,
            ReadFile(LogFileFor("FATAL")).find("kept-error"));
}

TEST_F(LoggingTest, LogIfDoesNotEvaluateDisabledBranch) {
  bool evaluated = false;
  LOG_IF(INFO, false) << (evaluated = true);
  EXPECT_FALSE(evaluated);

  LOG_IF(INFO, true) << "log-if-message";
}

TEST_F(LoggingTest, LogEveryNEvaluatesOnSchedule) {
  for (int i = 0; i < 3; ++i) {
    LOG_EVERY_N(INFO, 2) << "every-n-message";
  }
  rime::ShutdownLogging();

  const std::string content = ReadFile(LogFileFor("INFO"));
  size_t count = 0;
  for (size_t pos = 0;
       (pos = content.find("every-n-message", pos)) != std::string::npos;
       ++pos) {
    ++count;
  }
  EXPECT_EQ(2u, count);
}

TEST_F(LoggingTest, VlogIsOffUntilEnabled) {
  EXPECT_FALSE(rime::VlogIsOn(1));
  VLOG(1) << "vlog-disabled";

  rime::SetVlogLevel(2);
  EXPECT_TRUE(rime::VlogIsOn(2));
  EXPECT_FALSE(rime::VlogIsOn(3));
  VLOG(2) << "vlog-enabled";
  rime::SetVlogLevel(0);
  EXPECT_FALSE(rime::VlogIsOn(1));

  rime::ShutdownLogging();
  const std::string content = ReadFile(LogFileFor("INFO"));
  EXPECT_EQ(std::string::npos, content.find("vlog-disabled"));
  EXPECT_NE(std::string::npos, content.find("vlog-enabled"));
}

TEST_F(LoggingTest, MirrorsErrorsToStderrWhenLoggingToFile) {
#if !GTEST_HAS_STREAM_REDIRECTION
  GTEST_SKIP() << "stderr redirection is not supported";
#elif defined(_WIN32)
  // gtest redirects the CRT file descriptor, while spdlog's Windows color
  // sink writes directly to the Win32 STD_ERROR_HANDLE.  Redirect the handle
  // and rebuild the logger so the sink is created with the capture handle.
  rime::ShutdownLogging();

  const fs::path capture_path = log_dir_ / "captured-stderr.txt";
  std::string captured;
  {
    ScopedWin32Handle capture(
        ::CreateFileW(capture_path.wstring().c_str(), GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    ASSERT_NE(INVALID_HANDLE_VALUE, capture.get())
        << "failed to create stderr capture file";

    {
      ScopedStderrHandle redirect(capture.get());
      ASSERT_TRUE(redirect.replaced()) << "failed to redirect stderr";

      rime::InitializeLogging("rime.logging.test", 0,
                              log_dir_.string().c_str());
      LOG(INFO) << "info-not-mirrored";
      LOG(ERROR) << "error-mirrored";
      rime::ShutdownLogging();
    }

    ::FlushFileBuffers(capture.get());
  }
  captured = ReadFile(capture_path);

  EXPECT_EQ(std::string::npos, captured.find("info-not-mirrored"));
  EXPECT_NE(std::string::npos, captured.find("error-mirrored"));
#else
  testing::internal::CaptureStderr();
  LOG(INFO) << "info-not-mirrored";
  LOG(ERROR) << "error-mirrored";
  const std::string captured = testing::internal::GetCapturedStderr();

  EXPECT_EQ(std::string::npos, captured.find("info-not-mirrored"));
  EXPECT_NE(std::string::npos, captured.find("error-mirrored"));
#endif
}

TEST_F(LoggingTest, EmptyLogDirLogsToStderr) {
  rime::ShutdownLogging();
  rime::InitializeLogging("rime.logging.test", 0, "");
  EXPECT_TRUE(rime::IsLogToStderr());
  EXPECT_TRUE(rime::GetLoggingDirectories().empty());
  rime::ShutdownLogging();
}

TEST_F(LoggingTest, NullLogDirReportsCandidates) {
  rime::ShutdownLogging();
  rime::InitializeLogging("rime.logging.test", 0, nullptr);
  EXPECT_FALSE(rime::GetLoggingDirectories().empty());
  rime::ShutdownLogging();
}

#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
TEST_F(LoggingTest, FatalFlushesAndAborts) {
  EXPECT_DEATH(LOG(FATAL) << "fatal-message", "fatal-message");

  const std::string fatal_content = ReadFile(LogFileFor("FATAL"));
  const std::string error_content = ReadFile(LogFileFor("ERROR"));
  const std::string warning_content = ReadFile(LogFileFor("WARNING"));
  const std::string info_content = ReadFile(LogFileFor("INFO"));
  EXPECT_NE(std::string::npos, fatal_content.find("fatal-message"));
  EXPECT_NE(std::string::npos, error_content.find("fatal-message"));
  EXPECT_NE(std::string::npos, warning_content.find("fatal-message"));
  EXPECT_NE(std::string::npos, info_content.find("fatal-message"));
}
#endif

}  // namespace

#endif  // RIME_ENABLE_LOGGING
