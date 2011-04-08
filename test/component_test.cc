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

// define an interface to a family of interchangeable objects
// a nested Component class will be generated
class Greeting : public Class_<Greeting, const std::string&> {
 public:
  virtual const std::string Salut() = 0;
};

// define an implementation
class Hello : public Greeting {
 public:
  Hello(const std::string &title) : title_(title) {}
  const std::string Salut() {
    return "hello, " + title_ + ".";
  }
 private:
  std::string title_;
};

class HelloComponent : public Component_<Hello> {};

TEST(RimeComponentTest, ComponentCreation) {
  Component::Register("hello", new HelloComponent());
  Greeting::Component* greeting_component = Greeting::Find("hello");
  EXPECT_TRUE(greeting_component);

  scoped_ptr<Greeting> hello(greeting_component->Create("fred"));
  EXPECT_STREQ("hello, fred.", hello->Salut().c_str());
}

TEST(RimeComponentTest, UnknownComponentCreation) {
  // unregistered component class
  EXPECT_FALSE(Component::ByName("unknown"));
}

