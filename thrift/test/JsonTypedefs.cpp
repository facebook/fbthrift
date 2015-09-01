#include <common/init/Init.h>
#include <gtest/gtest.h>
#include <thrift/test/gen-cpp/JsonReaderTest_types.h>

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::initFacebook(&argc, &argv);
  return RUN_ALL_TESTS();
}

TEST(JsonTypedefs, test) {
  JsonTypedefs a;
  a.readFromJson("{\"x\":{\"1\":11}, \"y\":[2], \"z\":[3], \"w\":4}");
  EXPECT_FALSE(a.x.empty());
  EXPECT_FALSE(a.y.empty());
  EXPECT_FALSE(a.z.empty());
  EXPECT_NE(a.w, 0);
}
