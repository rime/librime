//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/deployer.h>
#include <rime/lever/deployment_tasks.h>
#include <filesystem>
#include <fstream>
#include <system_error>
#include "rime/config/config_component.h"
#include "rime/registry.h"

namespace fs = std::filesystem;
using namespace rime;

class RimeDeploymentTasksTest : public ::testing::Test {
 protected:
  RimeDeploymentTasksTest() = default;

  void CleanUp() {
    std::error_code ec;
    fs::remove("installation.yaml", ec);
  }
  virtual void SetUp() { CleanUp(); }
  virtual void TearDown() { CleanUp(); }
};

TEST_F(RimeDeploymentTasksTest, InstallationUpdate) {
  Registry& r = Registry::instance();
  r.Register("installation_update", new Component<InstallationUpdate>);
  // initialization
  string sync_dir;
  {
    rime::Deployer deployer;
    deployer.distribution_code_name = "rime-test";
    deployer.distribution_name = "rime-test";
    deployer.distribution_version = "1.0.0";
    EXPECT_TRUE(
        deployer.RunTask("installation_update", rime::TaskInitializer()));
    Config config;
    EXPECT_TRUE(config.LoadFromFile(path("installation.yaml")));
    EXPECT_EQ(config["distribution_code_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_version"].ToString(), "1.0.0");
    EXPECT_TRUE(config.HasKey("backup_config_files"));
    EXPECT_TRUE(config.HasKey("install_time"));
    EXPECT_TRUE(config.HasKey("installation_id"));
    EXPECT_TRUE(config.HasKey("rime_version"));
    EXPECT_TRUE(config.HasKey("sync_dir"));
    sync_dir = config["sync_dir"].ToString();
    EXPECT_EQ(path(sync_dir), path(".") / "sync");
    EXPECT_FALSE(config.HasKey("update_time"));
  }
  // update
  string update_time;
  {
    rime::Deployer deployer;
    deployer.distribution_code_name = "rime-test";
    deployer.distribution_name = "rime-test";
    deployer.distribution_version = "2.0.0";
    EXPECT_TRUE(
        deployer.RunTask("installation_update", rime::TaskInitializer()));
    Config config;
    EXPECT_TRUE(config.LoadFromFile(path("installation.yaml")));
    EXPECT_EQ(config["distribution_code_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_version"].ToString(), "2.0.0");
    EXPECT_TRUE(config.HasKey("backup_config_files"));
    EXPECT_TRUE(config.HasKey("install_time"));
    EXPECT_TRUE(config.HasKey("installation_id"));
    EXPECT_TRUE(config.HasKey("rime_version"));
    EXPECT_TRUE(config.HasKey("sync_dir"));
    EXPECT_EQ(config["sync_dir"].ToString(), sync_dir);
    EXPECT_TRUE(config.HasKey("update_time"));
    update_time = config["update_time"].ToString();
    EXPECT_TRUE(!update_time.empty());
  }
  // no update
  {
    rime::Deployer deployer;
    deployer.distribution_code_name = "rime-test";
    deployer.distribution_name = "rime-test";
    deployer.distribution_version = "2.0.0";
    EXPECT_TRUE(
        deployer.RunTask("installation_update", rime::TaskInitializer()));
    Config config;
    EXPECT_TRUE(config.LoadFromFile(path("installation.yaml")));
    EXPECT_EQ(config["update_time"].ToString(), update_time);
    EXPECT_TRUE(config.HasKey("sync_dir"));
    EXPECT_EQ(config["sync_dir"].ToString(), sync_dir);
  }
}

TEST_F(RimeDeploymentTasksTest, InstallationUpdateRecover) {
  Registry& r = Registry::instance();
  r.Register("installation_update", new Component<InstallationUpdate>);
  // write a corrupted installation.yaml
  {
    std::ofstream fout("installation.yaml");
    fout << "|\n  abc";  // root node is a scalar
    fout.close();
    Config config;
    config.LoadFromFile(path("installation.yaml"));
    EXPECT_TRUE(config.IsValue(""));
  }
  {
    rime::Deployer deployer;
    deployer.distribution_code_name = "rime-test";
    deployer.distribution_name = "rime-test";
    deployer.distribution_version = "2.0.0";
    EXPECT_TRUE(
        deployer.RunTask("installation_update", rime::TaskInitializer()));
    Config config;
    EXPECT_TRUE(config.LoadFromFile(path("installation.yaml")));
    EXPECT_TRUE(config.LoadFromFile(path("installation.yaml")));
    EXPECT_EQ(config["distribution_code_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_name"].ToString(), "rime-test");
    EXPECT_EQ(config["distribution_version"].ToString(), "2.0.0");
    EXPECT_TRUE(config.HasKey("backup_config_files"));
    EXPECT_TRUE(config.HasKey("install_time"));
    EXPECT_TRUE(config.HasKey("installation_id"));
    EXPECT_TRUE(config.HasKey("rime_version"));
    EXPECT_TRUE(config.HasKey("sync_dir"));
    EXPECT_FALSE(config.HasKey("update_time"));  // as if it is newly installed
  }
}
