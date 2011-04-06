#include <rime/common.h>
#include <rime/component.h>
#include <gtest/gtest.h>

using namespace rime;

class Hello : public Component {
};

class HelloClass : public ComponentClass {
public:
  virtual shared_ptr<Component> CreateInstance(Engine *engine) {
    return shared_ptr<Component>(new Hello);
  }
  virtual const std::string name() const { return "hello"; }
};

TEST(RimeComponentTest, ComponentCreation) {
  HelloClass* hello_klass(new HelloClass);
  EXPECT_STREQ("hello", hello_klass->name().c_str());
  hello_klass->Register();
  shared_ptr<Hello> hello = dynamic_pointer_cast<Hello>(Component::Create("hello", NULL));
  EXPECT_TRUE(hello);
}

TEST(RimeComponentTest, UnknownComponentCreation) {
  EXPECT_FALSE(Component::Create("unknown", NULL));
}

