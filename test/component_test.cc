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

typedef std::pair<std::string, std::string> GreetingInfo;

class Greeting : public Class_<Greeting, const std::string&> {
 public:
  virtual const std::string Salute() = 0;
};

class Hello : public Greeting {
 public:
  Hello(const GreetingInfo &info) : message_(info.first), name_(info.second) {}
  const std::string Salute() {
    return message_ + ", " + name_ + "!";
  }
 private:
  std::string message_;
  std::string name_;
};

// a customized component that has state
class HelloComponent : public Hello::Component {
 public:
  HelloComponent(const std::string &message) : message_(message) {}
  // define a custom creator to provide an additional argument for Hello ctor
  Hello* Create(const std::string &name) {
    return new Hello(std::make_pair(message_, name));
  }
 private:
  const std::string message_;
};


TEST(RimeComponentTest, ComponentCreation) {
  Component::Register("hello", new HelloComponent("hello"));
  Component::Register("goodbye", new HelloComponent("goodbye"));
  Greeting::Component* gc = Greeting::Find("hello");
  EXPECT_TRUE(gc);
  scoped_ptr<Greeting> hello(gc->Create("fred"));
  scoped_ptr<Greeting> goodbye(Greeting::Find("goodbye")->Create("michael"));
  EXPECT_STREQ("hello, fred!", hello->Salute().c_str());
  EXPECT_STREQ("goodbye, michael!", goodbye->Salute().c_str());
}

TEST(RimeComponentTest, UnknownComponentCreation) {
  // unregistered component class
  EXPECT_FALSE(Component::ByName("unknown"));
}

