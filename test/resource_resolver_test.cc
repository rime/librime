#include <gtest/gtest.h>
#include <rime/resource_resolver.h>

using namespace rime;

TEST(RimeResourceResolverTest, ResolvePath) {
  const auto type = ResourceType{
    "minerals",
    "not_",
    ".minerals",
  };
  the<ResourceResolver> rr(new ResourceResolver(type));
  rr->set_root_path("/starcraft");
  auto actual = rr->ResolvePath("enough");
  boost::filesystem::path expected = "/starcraft/not_enough.minerals";
  EXPECT_TRUE(expected == actual);
}
