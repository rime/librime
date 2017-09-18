#include <fstream>
#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <rime/resource.h>

using namespace rime;

static const ResourceType kMineralsType = ResourceType{
  "minerals",
  "not_",
  ".minerals",
};

TEST(RimeResourceResolverTest, ResolvePath) {
  ResourceResolver rr(kMineralsType);
  rr.set_root_path("/starcraft");
  auto actual = rr.ResolvePath("enough");
  boost::filesystem::path expected = boost::filesystem::system_complete(boost::filesystem::current_path()).root_name().string() + "/starcraft/not_enough.minerals";
  EXPECT_TRUE(actual.is_absolute());
  EXPECT_TRUE(expected == actual);
}

TEST(RimeResourceResolverTest, FallbackRootPath) {
  FallbackResourceResolver rr(kMineralsType);
  rr.set_fallback_root_path("fallback");
  boost::filesystem::create_directory("fallback");
  {
    boost::filesystem::path nonexistent_default = "not_present.minerals";
    boost::filesystem::remove(nonexistent_default);
    auto fallback = boost::filesystem::absolute("fallback/not_present.minerals");
    std::ofstream(fallback.string()).close();
    auto actual = rr.ResolvePath("present");
    EXPECT_TRUE(fallback == actual);
    boost::filesystem::remove(fallback);
  }
  {
    auto existent_default = boost::filesystem::absolute("not_falling_back.minerals");
    std::ofstream(existent_default.string()).close();
    auto actual = rr.ResolvePath("falling_back");
    EXPECT_TRUE(existent_default == actual);
    boost::filesystem::remove(existent_default);
  }
  {
    auto nonexistent_default = boost::filesystem::absolute("not_any.minerals");
    boost::filesystem::remove(nonexistent_default);
    auto nonexistent_fallback = boost::filesystem::absolute("fallback/not_any.minerals");
    boost::filesystem::remove(nonexistent_fallback);
    auto actual = rr.ResolvePath("any");
    EXPECT_TRUE(nonexistent_default == actual);
  }
  boost::filesystem::remove_all("fallback");
}
