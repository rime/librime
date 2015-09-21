//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-07 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/component.h>

using namespace rime;

class Greeting : public Class<Greeting, const string&> {
 public:
  virtual string Say() = 0;
  virtual ~Greeting() = default;
};

using HelloMessage = pair<string, string>;

class Hello : public Greeting {
 public:
  Hello(const HelloMessage& msg) : word_(msg.first), name_(msg.second) {
  }
  string Say() {
    return word_ + ", " + name_ + "!";
  }
 private:
  string word_;
  string name_;
};

// customize a hello component with parameters
class HelloComponent : public Hello::Component {
 public:
  HelloComponent(const string& word) : word_(word) {}
  // define a custom creator to provide an additional argument
  Hello* Create(const string& name) {
    return new Hello(make_pair(word_, name));
  }
 private:
  string word_;
};


TEST(RimeComponentTest, UsingComponent) {
  Registry& r = Registry::instance();
  r.Register("test_hello", new HelloComponent("hello"));
  r.Register("test_morning", new HelloComponent("good morning"));

  Greeting::Component* h = Greeting::Require("test_hello");
  EXPECT_TRUE(h != NULL);
  Greeting::Component* gm = Greeting::Require("test_morning");
  EXPECT_TRUE(gm != NULL);

  the<Greeting> g(gm->Create("michael"));
  EXPECT_STREQ("good morning, michael!", g->Say().c_str());

  r.Unregister("test_hello");
  r.Unregister("test_morning");
}

TEST(RimeComponentTest, UnknownComponent) {
  // unregistered component class
  EXPECT_FALSE(Registry::instance().Find("test_unknown"));
}
