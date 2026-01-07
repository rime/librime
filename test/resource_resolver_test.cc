#include <fstream>
#include <filesystem>
#include <gtest/gtest.h>
#include <rime/resource.h>

namespace fs = std::filesystem;
using namespace rime;

static const ResourceType kMineralsType = ResourceType{
    "minerals",
    "not_",
    ".minerals",
};

TEST(RimeResourceResolverTest, ResolvePath) {
  ResourceResolver rr(kMineralsType);
  rr.set_root_path(path{"/starcraft"});
  auto actual = rr.ResolvePath("enough");
  path expected = fs::absolute(fs::current_path())
                      .root_name()
                      .concat("/starcraft/not_enough.minerals");
  EXPECT_TRUE(actual.is_absolute());
  EXPECT_TRUE(expected == actual);
}

TEST(RimeResourceResolverTest, FallbackRootPath) {
  FallbackResourceResolver rr(kMineralsType);
  rr.set_fallback_root_path(path{"fallback"});
  fs::create_directory("fallback");
  {
    path nonexistent_default("not_present.minerals");
    fs::remove(nonexistent_default);
    auto fallback = fs::absolute("fallback/not_present.minerals");
    std::ofstream(fallback.string()).close();
    auto actual = rr.ResolvePath("present");
    EXPECT_TRUE(fallback == actual);
    fs::remove(fallback);
  }
  {
    auto existent_default = fs::absolute("not_falling_back.minerals");
    std::ofstream(existent_default.string()).close();
    auto actual = rr.ResolvePath("falling_back");
    EXPECT_TRUE(existent_default == actual);
    fs::remove(existent_default);
  }
  {
    auto nonexistent_default = fs::absolute("not_any.minerals");
    fs::remove(nonexistent_default);
    auto nonexistent_fallback = fs::absolute("fallback/not_any.minerals");
    fs::remove(nonexistent_fallback);
    auto actual = rr.ResolvePath("any");
    EXPECT_TRUE(nonexistent_default == actual);
  }
  fs::remove_all("fallback");
}
