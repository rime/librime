// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include <fstream>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/config.h>

namespace rime {

// private classes
namespace Types{
  enum { Null = 0, Scalar, Sequence, Map };
};

struct ConfigTreeNode {
  int type;
  std::vector<shared_ptr<ConfigTreeNode> > seq_children;
  std::map<std::string, shared_ptr<ConfigTreeNode> > map_children;
  std::string value;

  shared_ptr<ConfigTreeNode> FindValue( const std::string & key);
  shared_ptr<ConfigTreeNode> Clone();
};

shared_ptr<ConfigTreeNode> ConfigTreeNode::FindValue( const std::string & key){
  std::map<std::string, shared_ptr<ConfigTreeNode> >::iterator i = map_children.find(key);
  if(i != map_children.end())
    return i->second;
  else
    return shared_ptr<ConfigTreeNode>();
}

////
//shared_ptr<ConfigTreeNode> ConfigTreeNode::Clone(){
//
//}

class ConfigTree {
public:
  ConfigTree(){};
  ~ConfigTree(){};
  void CopyTree(const YAML::Node *node);
  std::string EmitTree();
  shared_ptr<ConfigTreeNode> root(){ return root_; };

private:
  YAML::Emitter emitter;
  shared_ptr<ConfigTreeNode> root_;
  void EmitSubTree(shared_ptr<ConfigTreeNode> node);
  void CopySubTree(const YAML::Node *node, shared_ptr<ConfigTreeNode> to);
};

void ConfigTree::CopySubTree(const YAML::Node *node, shared_ptr<ConfigTreeNode> to){
  if (!node || YAML::NodeType::Null == node->Type())
    return;

  if(YAML::NodeType::Scalar == node->Type()){
    to->type = Types::Scalar;
    to->value = node->to<std::string>();
  }
  else if(YAML::NodeType::Sequence == node->Type()){
    std::vector<shared_ptr<ConfigTreeNode> > seq_children;
    YAML::Iterator it = node->begin();
    YAML::Iterator end = node->end();
    size_t index = 0;
    to->type = Types::Sequence;

    for( ; it != end; ++it){
      shared_ptr<ConfigTreeNode> p(new ConfigTreeNode());

      CopySubTree(&(*it), p);
      seq_children.push_back(p);
    }
    
    to->seq_children = seq_children;
  }
  else if(YAML::NodeType::Map == node->Type()){
    std::map<std::string, shared_ptr<ConfigTreeNode> > map_children;
    YAML::Iterator it = node->begin();
    YAML::Iterator end = node->end();

    to->type = Types::Map;

    for( ; it != end; ++it){
      shared_ptr<ConfigTreeNode> p(new ConfigTreeNode());
      std::string key = it.first().to<std::string>();
      const YAML::Node *yaml_node = &(it.second());

      CopySubTree(yaml_node, p);
      map_children[key] = p;
    }

    to->map_children = map_children;
  }
}

void ConfigTree::CopyTree(const YAML::Node *node){
  if(!root_){
    root_.reset(new ConfigTreeNode());
  }
  
  CopySubTree(node, root_);
}

void ConfigTree::EmitSubTree(shared_ptr<ConfigTreeNode> node){
  if (!node)
  {
    return;
  }

  if(node->type == Types::Scalar){
    emitter << node->value;
  }
  else if(node->type == Types::Sequence){
    std::vector<shared_ptr<ConfigTreeNode> >::iterator it = node->seq_children.begin();
    std::vector<shared_ptr<ConfigTreeNode> >::iterator end = node->seq_children.end();
    emitter << YAML::BeginSeq;
    for( ; it != end; ++it){
      EmitSubTree(*it);
    }
    emitter << YAML::EndSeq;
  }
  else if(node->type == Types::Map){
    std::map<std::string, shared_ptr<ConfigTreeNode> >::iterator it = node->map_children.begin();
    std::map<std::string, shared_ptr<ConfigTreeNode> >::iterator end = node->map_children.end();
    emitter << YAML::BeginMap;
    for( ; it != end; ++it){
      emitter << YAML::Key << it->first;
      emitter << YAML::Value;
      EmitSubTree(it->second);
    }
    emitter << YAML::EndMap;
  }
}

std::string ConfigTree::EmitTree(){
  EmitSubTree(root_);
  return emitter.c_str();
}

// TODO:
class ConfigItemData {
 public:
  ConfigItemData() {}
  ConfigItemData(shared_ptr<ConfigTreeNode> node) : node_(node) {}
  shared_ptr<ConfigTreeNode> node() const { return node_; }
 private:
  shared_ptr<ConfigTreeNode> node_;
};

class ConfigData {
 public:
  ConfigData():config_tree(new ConfigTree()){};
  ~ConfigData(){ delete config_tree; };
  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);
  shared_ptr<ConfigTreeNode> Traverse(const std::string &key);

  static const ConfigItemPtr Convert(shared_ptr<ConfigTreeNode> node);

 private:
  YAML::Node doc_;
  ConfigTree *config_tree;
};

// ConfigItem members

ConfigItem::~ConfigItem() {
  if (data_) {
    delete data_;
    data_ = NULL;
  }
}

bool ConfigItem::GetBool(bool *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = boost::lexical_cast<bool> (data_->node()->value);
  return true;
}

bool ConfigItem::GetInt(int *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = boost::lexical_cast<int> (data_->node()->value);
  return true;
}

bool ConfigItem::GetDouble(double *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = boost::lexical_cast<double> (data_->node()->value);
  return true;
}

bool ConfigItem::GetString(std::string *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = data_->node()->value;
  return true;
}

bool ConfigItem::SetBool(bool value) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->value = value? "true" : "false";
  return true;
}

bool ConfigItem::SetInt(int value) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->value = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigItem::SetDouble(double value) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->value = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigItem::SetString(const std::string &value) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->value = value;
  return true;
}


// ConfigList members

ConfigItemPtr ConfigList::GetAt(size_t i) {
  shared_ptr<ConfigTreeNode> node = data_->node();
  if(node->type == Types::Sequence)
  {
    shared_ptr<ConfigTreeNode> p = node->seq_children[i];
    ConfigItemPtr ptr(ConfigData::Convert(p));
    return ptr;
  }
  else
  {
    return ConfigItemPtr();
  }
}

bool ConfigList::SetAt(size_t i, const ConfigItemPtr element) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->seq_children[i] = element->data()->node();
  return true;
}

bool ConfigList::Append(const ConfigItemPtr element) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->seq_children.push_back(element->data()->node());
  return true;
}

bool ConfigList::Clear() {
  if(!data_ || !data_->node())
    return false;
  data_->node()->seq_children.clear();
  return true;
}

size_t ConfigList::size() const {
  shared_ptr<ConfigTreeNode> node = data_->node();
  if(node->type == Types::Sequence)
    return node->seq_children.size();
  else
  {
    return 0;
  }
}

// ConfigMap members

bool ConfigMap::HasKey(const std::string &key) const {
  shared_ptr<ConfigTreeNode> node = data_->node();
  if(node->type == Types::Map)
  {
    shared_ptr<ConfigTreeNode> p = node->FindValue(key);
    return p != NULL;
  }
  else
  {
    return false;
  }
}

ConfigItemPtr ConfigMap::Get(const std::string &key) {
  shared_ptr<ConfigTreeNode> node = data_->node();
  if(node->type == Types::Map)
  {
    shared_ptr<ConfigTreeNode> p = node->FindValue(key);
    return ConfigItemPtr(ConfigData::Convert(p));
  }
  else
  {
    return ConfigItemPtr();
  }
}

bool ConfigMap::Set(const std::string &key, const ConfigItemPtr element) {
  if(!data_ || !data_->node())
    return false;
  data_->node()->map_children[key] = element->data()->node();
  return true;
}

bool ConfigMap::Clear() {
  if(!data_ || !data_->node())
    return false;
  data_->node()->map_children.clear();
  return true;
}

// Config members

Config::Config() : data_(new ConfigData) {
}

Config::~Config() {
}

Config::Config(const std::string &file_name) {
  data_ = ConfigDataManager::instance().GetConfigData(file_name);
}

bool Config::LoadFromFile(const std::string& file_name) {
  return data_->LoadFromFile(file_name);
}

bool Config::SaveToFile(const std::string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const std::string &key) {
  EZLOGGERVAR(key);
  shared_ptr<ConfigTreeNode> p = data_->Traverse(key);
  return !p ||p->type == Types::Null;
}

bool Config::GetBool(const std::string& key, bool *value) {
  EZLOGGERVAR(key);
  shared_ptr<ConfigTreeNode> p = data_->Traverse(key);
  if (!p ||p->type != Types::Scalar)
    return false;
  std::string bstr = p->value;
  boost::to_lower(bstr);

  if("true" == bstr)
    *value = true;
  else if("false" == bstr)
    *value = false;
  else
    return false;

  return true;
}

bool Config::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  shared_ptr<ConfigTreeNode> p = data_->Traverse(key);
  if (!p ||p->type != Types::Scalar)
    return false;
  *value = boost::lexical_cast<int> (p->value);
  return true;
}

bool Config::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  shared_ptr<ConfigTreeNode> p = data_->Traverse(key);
  if (!p ||p->type != Types::Scalar)
    return false;
  *value = boost::lexical_cast<double> (p->value);
  return true;
}

bool Config::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  shared_ptr<ConfigTreeNode> p = data_->Traverse(key);
  if (!p ||p->type != Types::Scalar)
    return false;
  *value = p->value;
  return true;
}

shared_ptr<ConfigList> Config::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(ConfigData::Convert(data_->Traverse(key)));
  return dynamic_pointer_cast<ConfigList>(p);
}

shared_ptr<ConfigMap> Config::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(ConfigData::Convert(data_->Traverse(key)));
  return dynamic_pointer_cast<ConfigMap>(p);
}

// ConfigComponent members

const std::string ConfigComponent::GetConfigFilePath(const std::string &config_id) {
  return boost::str(boost::format(pattern_) % config_id);
}

Config* ConfigComponent::Create(const std::string &config_id) {
  const std::string path(GetConfigFilePath(config_id));
  EZLOGGERPRINT("config file path: %s", path.c_str());
  return new Config(path);
}

// ConfigDataManager memebers

scoped_ptr<ConfigDataManager> ConfigDataManager::instance_;

shared_ptr<ConfigData> ConfigDataManager::GetConfigData(const std::string &config_file_path) {
  shared_ptr<ConfigData> sp;
  // keep a weak reference to the shared config data in the manager
  weak_ptr<ConfigData> &wp((*this)[config_file_path]);
  if (wp.expired()) {  // create a new copy and load it
    sp.reset(new ConfigData);
    sp->LoadFromFile(config_file_path);
    wp = sp;
  }
  else {  // obtain the shared copy
    sp = wp.lock();
  }
  return sp;
}

bool ConfigDataManager::ReloadConfigData(const std::string &config_file_path) {
  iterator it = find(config_file_path);
  if (it == end()) {  // never loaded
    return false;
  }
  shared_ptr<ConfigData> sp = it->second.lock();
  if (!sp)  {  // already been freed
    erase(it);
    return false;
  }
  sp->LoadFromFile(config_file_path);
  return true;
}

// ConfigData members

bool ConfigData::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  bool result = parser.GetNextDocument(doc_);

  //Clear the previous tree
  if(config_tree)
    delete config_tree;
  config_tree = new ConfigTree();
  //Load from YAML
  config_tree->CopyTree(&doc_);

  return result;
}

bool ConfigData::SaveToFile(const std::string& file_name) {
  std::ofstream out(file_name);  
  out << config_tree->EmitTree();
  return true;
}

const ConfigItemPtr ConfigData::Convert(shared_ptr<ConfigTreeNode> node) {
  if (!node)
    return ConfigItemPtr();
  // no need to recursively convert YAML::Node structure,
  // just wrap the node itself...
  // we can wrap its children nodes when they are retrived via getters
  int type = node->type;
  if (type == Types::Scalar) {
    return ConfigItemPtr(new ConfigItem(ConfigItem::kScalar,
                                        new ConfigItemData(node)));
  }
  if (type == Types::Sequence) {
    EZDBGONLYLOGGERPRINT("sequence size: %d", node->seq_children.size());
    return ConfigItemPtr(new ConfigList(new ConfigItemData(node)));
  }
  if (type == Types::Map) {
    return ConfigItemPtr(new ConfigMap(new ConfigItemData(node)));
  }
  return ConfigItemPtr();
}

shared_ptr<ConfigTreeNode> ConfigData::Traverse(const std::string &key) {
  EZDBGONLYLOGGERPRINT("traverse: %s", key.c_str());
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  // find the YAML::Node, and wrap it!
  shared_ptr<ConfigTreeNode> p = config_tree->root();
  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator end = keys.end();
  for (; it != end; ++it) {
    EZDBGONLYLOGGERPRINT("key node: %s", it->c_str());
    if (!p || Types::Null ==p->type)
      return shared_ptr<ConfigTreeNode>();
    p = p->FindValue(*it);
  }
  return p;
}

}  // namespace rime
