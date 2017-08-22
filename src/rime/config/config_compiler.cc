#include <boost/algorithm/string.hpp>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

struct Dependency {
  an<ConfigItemRef> target;

  virtual bool blocking() const = 0;
  virtual bool Resolve(ConfigCompiler* compiler) = 0;
};

struct PendingChild : Dependency {
  string child_path;
  an<ConfigItemRef> child_ref;

  PendingChild(const string& path, const an<ConfigItemRef>& ref)
      : child_path(path), child_ref(ref) {
  }
  bool blocking() const override {
    return false;
  }
  bool Resolve(ConfigCompiler* compiler) override;
};

template <class StreamT>
StreamT& operator<< (StreamT& stream, const Reference& reference) {
  return stream << reference.resource_id << ":" << reference.local_path;
}

struct IncludeReference : Dependency {
  IncludeReference(const Reference& r) : reference(r) {
  }
  bool blocking() const override {
    return true;
  }
  bool Resolve(ConfigCompiler* compiler) override;

  Reference reference;
};

struct PatchReference : Dependency {
  PatchReference(const Reference& r) : reference(r) {
  }
  bool blocking() const override {
    return true;
  }
  bool Resolve(ConfigCompiler* compiler) override;

  Reference reference;
};

struct PatchLiteral : Dependency {
  an<ConfigMap> patch;

  PatchLiteral(an<ConfigMap> map) : patch(map) {
  }
  bool blocking() const override {
    return true;
  }
  bool Resolve(ConfigCompiler* compiler) override;
};

struct ConfigDependencyGraph {
  map<string, of<ConfigResource>> resources;
  vector<of<ConfigItemRef>> node_stack;
  vector<string> key_stack;
  map<string, list<of<Dependency>>> deps;

  void Add(an<Dependency> dependency);

  void Push(an<ConfigItemRef> item, const string& key) {
    node_stack.push_back(item);
    key_stack.push_back(key);
  }

  void Pop() {
    node_stack.pop_back();
    key_stack.pop_back();
  }

  string current_resource_id() const {
    return key_stack.empty() ? string()
        : boost::trim_right_copy_if(key_stack.front(), boost::is_any_of(":"));
  }
};

bool PendingChild::Resolve(ConfigCompiler* compiler) {
  return compiler->ResolveDependencies(child_path);
}

static an<ConfigItem> ResolveReference(ConfigCompiler* compiler,
                                       const Reference& reference);

bool IncludeReference::Resolve(ConfigCompiler* compiler) {
  LOG(INFO) << "IncludeReference::Resolve(reference = " << reference << ")";
  auto item = ResolveReference(compiler, reference);
  if (!item) {
    return false;
  }
  *target = item;
  return true;
}

bool PatchReference::Resolve(ConfigCompiler* compiler) {
  auto item = ResolveReference(compiler, reference);
  if (!item) {
    return false;
  }
  if (!Is<ConfigMap>(item)) {
    LOG(ERROR) << "invalid patch at " << reference;
    return false;
  }
  PatchLiteral patch{As<ConfigMap>(item)};
  patch.target = target;
  return patch.Resolve(compiler);
}

// defined in config_data.cc
bool TraverseCopyOnWrite(an<ConfigItemRef> root, const string& path,
                         an<ConfigItem> item);

bool PatchLiteral::Resolve(ConfigCompiler* compiler) {
  bool success = true;
  for (const auto& entry : *patch) {
    const auto& path = entry.first;
    const auto& value = entry.second;
    if (!TraverseCopyOnWrite(target, path, value)) {
      LOG(ERROR) << "error applying patch to " << path;
      success = false;
    }
  }
  return success;
}

void ConfigDependencyGraph::Add(an<Dependency> dependency) {
  LOG(INFO) << "ConfigDependencyGraph::Add(), node_stack.size() = " << node_stack.size();
  if (node_stack.empty()) return;
  const auto& target = node_stack.back();
  dependency->target = target;
  auto target_path = ConfigData::JoinPath(key_stack);
  deps[target_path].push_back(dependency);
  LOG(INFO) << "target_path = " << target_path << ", #deps = " << deps[target_path].size();
  // The current pending node becomes a prioritized dependency of parent node
  auto child = target;
  auto keys = key_stack;
  for (auto prev = node_stack.rbegin() + 1; prev != node_stack.rend(); ++prev) {
    auto last_key = keys.back();
    keys.pop_back();
    auto parent_path = ConfigData::JoinPath(keys);
    auto& parent_deps = deps[parent_path];
    bool parent_was_pending = !parent_deps.empty();
    // Pending children should be resolved before applying __include or __patch
    parent_deps.push_front(New<PendingChild>(parent_path + "/" + last_key, child));
    LOG(INFO) << "parent_path = " << parent_path << ", #deps = " << parent_deps.size();
    if (parent_was_pending) {
      // so was all ancestors
      break;
    }
    child = *prev;
  }
}

ConfigCompiler::ConfigCompiler(ResourceResolver* resource_resolver)
    : resource_resolver_(resource_resolver),
      graph_(new ConfigDependencyGraph) {
}

ConfigCompiler::~ConfigCompiler() {
}

Reference ConfigCompiler::CreateReference(const string& qualified_path) {
  auto separator = qualified_path.find_first_of(":");
  string resource_id = resource_resolver_->ToResourceId(
      (separator == string::npos || separator == 0) ?
      graph_->current_resource_id() : qualified_path.substr(0, separator));
  string local_path = (separator == string::npos) ?
      qualified_path :
      qualified_path.substr(separator + 1);
  return Reference{resource_id, local_path};
}

void ConfigCompiler::AddDependency(an<Dependency> dependency) {
  graph_->Add(dependency);
}

void ConfigCompiler::Push(an<ConfigList> config_list, size_t index) {
  graph_->Push(
      New<ConfigListEntryRef>(nullptr, config_list, index),
      ConfigData::FormatListIndex(index));
}

void ConfigCompiler::Push(an<ConfigMap> config_map, const string& key) {
  graph_->Push(
      New<ConfigMapEntryRef>(nullptr, config_map, key),
      key);
}

void ConfigCompiler::Pop() {
  graph_->Pop();
}

an<ConfigResource> ConfigCompiler::GetCompiledResource(
    const string& resource_id) const {
  return graph_->resources[resource_id];
}

an<ConfigResource> ConfigCompiler::Compile(const string& file_name) {
  auto resource_id = resource_resolver_->ToResourceId(file_name);
  auto resource = New<ConfigResource>(resource_id, New<ConfigData>());
  graph_->resources[resource_id] = resource;
  graph_->Push(resource, resource_id + ":");
  if (!resource->data->LoadFromFile(
          resource_resolver_->ResolvePath(resource_id).string(), this)) {
    resource.reset();
  }
  graph_->Pop();
  return resource;
}

static inline an<ConfigItem> if_resolved(ConfigCompiler* compiler,
                                         an<ConfigItem> item,
                                         const string& path) {
  return item && compiler->resolved(path) ? item : nullptr;
}

static bool ResolveBlockingDependencies(ConfigCompiler* compiler,
                                        const string& path) {
  if (!compiler->blocking(path)) {
    return true;
  }
  LOG(INFO) << "blocking node: " << path;
  if (compiler->ResolveDependencies(path)) {
    LOG(INFO) << "resolved blocking node:" << path;
    return true;
  }
  return false;
}

static an<ConfigItem> GetResolvedItem(ConfigCompiler* compiler,
                                      an<ConfigResource> resource,
                                      const string& path) {
  LOG(INFO) << "GetResolvedItem(" << resource->resource_id << ":/" << path << ")";
  string node_path = resource->resource_id + ":";
  if (!resource || compiler->blocking(node_path)) {
    return nullptr;
  }
  an<ConfigItem> result = *resource;
  if (path.empty() || path == "/") {
    return if_resolved(compiler, result, node_path);
  }
  vector<string> keys = ConfigData::SplitPath(path);
  for (const auto& key : keys) {
    if (Is<ConfigList>(result)) {
      if (ConfigData::IsListItemReference(key)) {
        size_t index = ConfigData::ResolveListIndex(result, key, true);
        (node_path += "/") += ConfigData::FormatListIndex(index);
        if (!ResolveBlockingDependencies(compiler, node_path)) {
          return nullptr;
        }
        result = As<ConfigList>(result)->GetAt(index);
      } else {
        result.reset();
      }
    } else if (Is<ConfigMap>(result)) {
      LOG(INFO) << "advance with key: " << key;
      (node_path += "/") += key;
      if (!ResolveBlockingDependencies(compiler, node_path)) {
        return nullptr;
      }
      result = As<ConfigMap>(result)->Get(key);
    } else {
      result.reset();
    }
    if (!result) {
      LOG(INFO) << "missing node: " << node_path;
      return nullptr;
    }
  }
  return if_resolved(compiler, result, node_path);
}

bool ConfigCompiler::blocking(const string& full_path) const {
  auto found = graph_->deps.find(full_path);
  return found != graph_->deps.end() && !found->second.empty()
      && found->second.back()->blocking();
}

bool ConfigCompiler::pending(const string& full_path) const {
  return !resolved(full_path);
}

bool ConfigCompiler::resolved(const string& full_path) const {
  auto found = graph_->deps.find(full_path);
  return found == graph_->deps.end() || found->second.empty();
}

static an<ConfigItem> ResolveReference(ConfigCompiler* compiler,
                                       const Reference& reference) {
  auto resource = compiler->GetCompiledResource(reference.resource_id);
  if (!resource) {
    LOG(INFO) << "resource not found, compiling: " << reference.resource_id;
    resource = compiler->Compile(reference.resource_id);
  }
  return GetResolvedItem(compiler, resource, reference.local_path);
}

// Includes contents of nodes at specified paths.
// __include: path/to/local/node
// __include: filename[.yaml]:/path/to/external/node
static bool ParseInclude(ConfigCompiler* compiler,
                         const an<ConfigItem>& item) {
  if (Is<ConfigValue>(item)) {
    auto path = As<ConfigValue>(item)->str();
    LOG(INFO) << "ParseInclude(" << path << ")";
    compiler->AddDependency(
        New<IncludeReference>(compiler->CreateReference(path)));
    return true;
  }
  return false;
}

// Applies `parser` to every list element if `item` is a list.
// __patch: [ first/patch, filename:/second/patch ]
// __patch: [{list/@next: 1}, {list/@next: 2}]
static bool ParseList(bool (*parser)(ConfigCompiler*, const an<ConfigItem>&),
                      ConfigCompiler* compiler,
                      const an<ConfigItem>& item) {
  if (Is<ConfigList>(item)) {
    for (auto list_item : *As<ConfigList>(item)) {
      if (!parser(compiler, list_item)) {
        return false;
      }
    }
    return true;
  }
  // not a list
  return parser(compiler, item);
}

// Modifies subnodes or list elements at specified paths.
// __patch: path/to/node
// __patch: filename[.yaml]:/path/to/node
// __patch: { key/alpha: value, key/beta: value }
static bool ParsePatch(ConfigCompiler* compiler,
                       const an<ConfigItem>& item) {
  if (Is<ConfigValue>(item)) {
    auto path = As<ConfigValue>(item)->str();
    LOG(INFO) << "ParsePatch(" << path << ")";
    compiler->AddDependency(
        New<PatchReference>(compiler->CreateReference(path)));
    return true;
  }
  if (Is<ConfigMap>(item)) {
    LOG(INFO) << "ParsePatch(<literal>)";
    compiler->AddDependency(New<PatchLiteral>(As<ConfigMap>(item)));
    return true;
  }
  return false;
}

bool ConfigCompiler::Parse(const string& key, const an<ConfigItem>& item) {
  LOG(INFO) << "ConfigCompiler::Parse(" << key << ")";
  if (key == INCLUDE_DIRECTIVE) {
    return ParseInclude(this, item);
  }
  if (key == PATCH_DIRECTIVE) {
    return ParseList(ParsePatch, this, item);
  }
  return false;
}

bool ConfigCompiler::Link(an<ConfigResource> target) {
  LOG(INFO) << "Link(" << target->resource_id << ")";
  auto found = graph_->resources.find(target->resource_id);
  if (found == graph_->resources.end()) {
    LOG(INFO) << "resource not found: " << target->resource_id;
    return false;
  }
  return ResolveDependencies(found->first + ":");
}

bool ConfigCompiler::ResolveDependencies(const string& path) {
  LOG(INFO) << "ResolveDependencies(" << path << ")";
  auto& deps = graph_->deps[path];
  for (auto iter = deps.begin(); iter != deps.end(); ) {
    if (!(*iter)->Resolve(this)) {
      LOG(INFO) << "unesolved dependency!";
      return false;
    }
    LOG(INFO) << "resolved.";
    iter = deps.erase(iter);
  }
  LOG(INFO) << "all dependencies resolved.";
  return true;
}

}  // namespace rime
