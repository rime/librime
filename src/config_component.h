// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIGCOMPONENT_H_
#define RIME_CONFIGCOMPONENT_H_

#include <rime/common.h>
#include <rime/component.h>
#include <yaml-cpp/yaml.h>

namespace rime {  

class ConfigComponent : public Component{
public:
  ConfigComponent(){}
  virtual ~ConfigComponent(){}
  void LoadFromFile(const std::string& fileName);
  void SaveToFile(const std::string& fileName);
  std::string GetValue(const std::string& keyPath);
  
private:
  YAML::Node doc;
};

class ConfigComponentClass : public ComponentClass {
public:
  virtual shared_ptr<Component> CreateInstance(Engine *engine) {
    return shared_ptr<Component>(new ConfigComponent);
  }
  virtual const std::string name() const { return "ConfigComponent"; }
};

}  //namespace rime

#endif