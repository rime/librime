// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-07 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/component.h>

using namespace rime;

class Hello : public Component {
};

class HelloClass : public ComponentClass {
 public:
  virtual Component* CreateInstance(Engine *engine) { return new Hello; }
  virtual const std::string name() const { return "hello"; }
};

TEST(RimeComponentTest, ComponentCreation) {
  // create hello component class
  HelloClass* hello_klass(new HelloClass);
  EXPECT_STREQ("hello", hello_klass->name().c_str());
  // register the component class
  hello_klass->Register();
  // CAVEAT: no delete since the class object is owned by the registry

  // create a hello component instance
  shared_ptr<Hello> hello = dynamic_pointer_cast<Hello>(
      Component::Create("hello", NULL));
  EXPECT_TRUE(hello);
}

TEST(RimeComponentTest, UnknownComponentCreation) {
  // unregistered component class
  EXPECT_FALSE(Component::Create("unknown", NULL));
}

