// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPONENT_H_
#define RIME_COMPONENT_H_

#include <string>
#include <rime/common.h>

namespace rime {

// desired usage
/*
// define an interface to a family of interchangeable objects
// a nested Component class will be generated
class Config : public Class_<Config, const std::string&> {
 public:
  virtual const std::wstring GetValue(const std::string &key) = 0;
};

// define an implementation
class YamlConfig : public Config {
 public:
  YamlConfig(const std::string &file_name) {}
  // ...
};

class YamlConfigComponent : public Component_<YamlConfig> {
 public:
  YamlConfigComponent(const std::string &conf_dir) {}
  // ...
};

void SampleUsage() {
  // register components
  Component::Register("global_config",
      new YamlConfigComponent("/appdata/rime/conf.d/"));
  Component::Register("user_config",
      new YamlConfigComponent("/user/.rime/conf.d/"));

  // find a component of a known klass
  Config::Component *uc = Config::Find("user_config");

  // call klass specific creators,
  // such as Config::Component::Create(std::string), 
  // Processor::Component::Create(Engine*) ...
  shared_ptr<Config> user_settings = uc->Create("settings.yaml");
  shared_ptr<Config> schema_settings = uc->Create("luna_pinyin.schema.yaml");
}
*/

class Component {
 public:
  Component() {}
  virtual ~Component() {}
  static void Register(const std::string& name, Component *component);
  static Component* ByName(const std::string& name);
};

template <class K, class Arg>
struct Class_ {
  typedef Arg Initializer;

  class Component : public rime::Component {
   public:
    virtual K* Create(const Initializer& arg) = 0;
  };

  static Component* Find(const std::string& name) {
    return dynamic_cast<Component*>(Component::ByName(name));
  }
};

template <class T>
struct Component_ : public T::Component {
 public:
  T* Create(const typename T::Initializer& arg) {
    return new T(arg);
  }
};

}  // namespace rime

#endif  // RIME_COMPONENT_H_
