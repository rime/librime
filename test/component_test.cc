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

class Greeting : public Class<Greeting, const std::string&> {
 public:
  virtual const std::string Say() = 0;
};

typedef std::pair<std::string, std::string> HelloMessage;

class Hello : public Greeting {
 public:
  Hello(const HelloMessage &msg) : word_(msg.first), name_(msg.second) {
  }
  const std::string Say() {
    return word_ + ", " + name_ + "!";
  }
 private:
  std::string word_;
  std::string name_;
};

// customize a hello component with parameters
class HelloComponent : public Hello::Component {
 public:
  HelloComponent(const std::string &word) : word_(word) {}
  // define a custom creator to provide an additional argument
  Hello* Create(const std::string &name) {
    return new Hello(std::make_pair(word_, name));
  }
 private:
  const std::string word_;
};


TEST(RimeComponentTest, UsingComponent) {
  Registry &r = Registry::instance();
  r.Register("test_hello", new HelloComponent("hello"));
  r.Register("test_morning", new HelloComponent("good morning"));

  Greeting::Component* h = Greeting::Require("test_hello");
  EXPECT_TRUE(h != NULL);
  Greeting::Component* gm = Greeting::Require("test_morning");
  EXPECT_TRUE(gm != NULL);

  scoped_ptr<Greeting> g(gm->Create("michael"));
  EXPECT_STREQ("good morning, michael!", g->Say().c_str());

  r.Unregister("test_hello");
  r.Unregister("test_morning");
}

TEST(RimeComponentTest, UnknownComponent) {
  // unregistered component class
  EXPECT_FALSE(Registry::instance().Find("test_unknown"));
}

