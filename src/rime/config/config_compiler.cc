#include <boost/algorithm/string.hpp>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

enum DependencyPriority {
  kPendingChild = 0,
  kInclude = 1,
  kPatch = 2,
};

struct Dependency {
  an<ConfigItemRef> target;

  virtual DependencyPriority priority() const = 0;
  bool blocking() const {
    return priority() > kPendingChild;
  }
  virtual string repr() const = 0;
  virtual bool Resolve(ConfigCompiler* compiler) = 0;
};

template <class StreamT>
StreamT& operator<< (StreamT& stream, const Dependency& dep) {
  return stream << dep.repr();
}

struct PendingChild : Dependency {
  string child_path;
  an<ConfigItemRef> child_ref;

  PendingChild(const string& path, const an<ConfigItemRef>& ref)
      : child_path(path), child_ref(ref) {
  }
  DependencyPriority priority() const override {
    return kPendingChild;
  }
  string repr() const override {
    return "PendingChild(" + child_path + ")";
  }
  bool Resolve(ConfigCompiler* compiler) override;
};

string Reference::repr() const {
  return resource_id + ":" + local_path + (optional ? " <optional>" : "");
}

template <class StreamT>
StreamT& operator<< (StreamT& stream, const Reference& reference) {
  return stream << reference.repr();
}

struct IncludeReference : Dependency {
  IncludeReference(const Reference& r) : reference(r) {
  }
  DependencyPriority priority() const override {
    return kInclude;
  }
  string repr() const override {
    return "Include(" + reference.repr() + ")";
  }
  bool Resolve(ConfigCompiler* compiler) override;

  Reference reference;
};

struct PatchReference : Dependency {
  PatchReference(const Reference& r) : reference(r) {
  }
  DependencyPriority priority() const override {
    return kPatch;
  }
  string repr() const override {
    return "Patch(" + reference.repr() + ")";
  }
  bool Resolve(ConfigCompiler* compiler) override;

  Reference reference;
};

struct PatchLiteral : Dependency {
  an<ConfigMap> patch;

  PatchLiteral(an<ConfigMap> map) : patch(map) {
  }
  DependencyPriority priority() const override {
    return kPatch;
  }
  string repr() const override {
    return "Patch(<literal>)";
  }
  bool Resolve(ConfigCompiler* compiler) override;
};

struct ConfigDependencyGraph {
  map<string, of<ConfigResource>> resources;
  vector<of<ConfigItemRef>> node_stack;
  vector<string> key_stack;
  map<string, vector<of<Dependency>>> deps;

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

static bool MergeTree(an<ConfigItemRef> target, an<ConfigMap> map);

bool IncludeReference::Resolve(ConfigCompiler* compiler) {
  DLOG(INFO) << "IncludeReference::Resolve(reference = " << reference << ")";
  auto included = ResolveReference(compiler, reference);
  if (!included) {
    return reference.optional;
  }
  // merge literal key-values into the included map
  auto overrides = As<ConfigMap>(**target);
  *target = included;
  if (overrides && !overrides->empty() && !MergeTree(target, overrides)) {
    LOG(ERROR) << "failed to merge tree: " << reference;
    return false;
  }
  return true;
}

bool PatchReference::Resolve(ConfigCompiler* compiler) {
  DLOG(INFO) << "PatchReference::Resolve(reference = " << reference << ")";
  auto item = ResolveReference(compiler, reference);
  if (!item) {
    return reference.optional;
  }
  if (!Is<ConfigMap>(item)) {
    LOG(ERROR) << "invalid patch at " << reference;
    return false;
  }
  PatchLiteral patch{As<ConfigMap>(item)};
  patch.target = target;
  return patch.Resolve(compiler);
}

static bool AppendToString(an<ConfigItemRef> target, an<ConfigValue> value) {
  if (!value)
    return false;
  auto existing_value = As<ConfigValue>(**target);
  if (!existing_value) {
    LOG(ERROR) << "trying to append string to non scalar";
    return false;
  }
  *target = existing_value->str() + value->str();
  return true;
}

static bool AppendToList(an<ConfigItemRef> target, an<ConfigList> list) {
  if (!list)
    return false;
  auto existing_list = As<ConfigList>(**target);
  if (!existing_list) {
    LOG(ERROR) << "trying to append list to other value";
    return false;
  }
  if (list->empty())
    return true;
  auto copy = New<ConfigList>(*existing_list);
  for (ConfigList::Iterator iter = list->begin(); iter != list->end(); ++iter) {
    if (!copy->Append(*iter))
      return false;
  }
  *target = copy;
  return true;
}

static bool EditNode(an<ConfigItemRef> target,
                     const string& key,
                     const an<ConfigItem>& value,
                     bool merge_tree);

static bool MergeTree(an<ConfigItemRef> target, an<ConfigMap> map) {
  if (!map)
    return false;
  // NOTE: the referenced content of target can be any type
  for (ConfigMap::Iterator iter = map->begin(); iter != map->end(); ++iter) {
    const auto& key = iter->first;
    const auto& value = iter->second;
    if (!EditNode(target, key, value, true)) {
      LOG(ERROR) << "error merging branch " << key;
      return false;
    }
  }
  return true;
}

static constexpr const char* ADD_SUFFIX_OPERATOR = "/+";
static constexpr const char* EQU_SUFFIX_OPERATOR = "/=";

inline static bool IsAppending(const string& key) {
  return key == ConfigCompiler::APPEND_DIRECTIVE ||
      boost::ends_with(key, ADD_SUFFIX_OPERATOR);
}
inline static bool IsMerging(const string& key,
                             const an<ConfigItem>& value,
                             bool merge_tree) {
  return key == ConfigCompiler::MERGE_DIRECTIVE ||
      boost::ends_with(key, ADD_SUFFIX_OPERATOR) ||
      (merge_tree && Is<ConfigMap>(value) &&
       !boost::ends_with(key, EQU_SUFFIX_OPERATOR));
}

inline static string StripOperator(const string& key, bool adding) {
  return (key == ConfigCompiler::APPEND_DIRECTIVE ||
          key == ConfigCompiler::MERGE_DIRECTIVE) ? "" :
      boost::erase_last_copy(
          key, adding ? ADD_SUFFIX_OPERATOR : EQU_SUFFIX_OPERATOR);
}

// defined in config_data.cc
bool TraverseCopyOnWrite(an<ConfigItemRef> root, const string& path,
                         function<bool (an<ConfigItemRef> target)> writer);

static bool EditNode(an<ConfigItemRef> target,
                     const string& key,
                     const an<ConfigItem>& value,
                     bool merge_tree) {
  DLOG(INFO) << "EditNode(" << key << "," << merge_tree << ")";
  bool appending = IsAppending(key);
  bool merging = IsMerging(key, value, merge_tree);
  auto writer = [=](an<ConfigItemRef> target) {
    if ((appending || merging) && **target) {
      DLOG(INFO) << "writer: editing node";
      return !value ||
      (appending && (AppendToString(target, As<ConfigValue>(value)) ||
                     AppendToList(target, As<ConfigList>(value)))) ||
      (merging && MergeTree(target, As<ConfigMap>(value)));
    } else {
      DLOG(INFO) << "writer: overwriting node";
      *target = value;
      return true;
    }
  };
  string path = StripOperator(key, appending || merging);
  DLOG(INFO) << "appending: " << appending << ", merging: " << merging
             << ", path: " << path;
  return TraverseCopyOnWrite(target, path, writer);
}

bool PatchLiteral::Resolve(ConfigCompiler* compiler) {
  DLOG(INFO) << "PatchLiteral::Resolve()";
  bool success = true;
  for (const auto& entry : *patch) {
    const auto& key = entry.first;
    const auto& value = entry.second;
    LOG(INFO) << "patching " << key;
    if (!EditNode(target, key, value, false)) {
      LOG(ERROR) << "error applying patch to " << key;
      success = false;
    }
  }
  return success;
}

static void InsertByPriority(vector<of<Dependency>>& list,
                             const an<Dependency>& value) {
  auto upper = std::upper_bound(
      list.begin(), list.end(), value,
      [](const an<Dependency>& lhs, const an<Dependency>& rhs) {
        return lhs->priority() < rhs->priority();
      });
  list.insert(upper, value);
}

void ConfigDependencyGraph::Add(an<Dependency> dependency) {
  DLOG(INFO) << "ConfigDependencyGraph::Add(), node_stack.size() = "
             << node_stack.size();
  if (node_stack.empty()) return;
  const auto& target = node_stack.back();
  dependency->target = target;
  auto target_path = ConfigData::JoinPath(key_stack);
  auto& target_deps = deps[target_path];
  bool target_was_pending = !target_deps.empty();
  InsertByPriority(target_deps, dependency);
  DLOG(INFO) << "target_path = " << target_path
             << ", #deps = " << target_deps.size();
  if (target_was_pending ||  // so was all ancestors
      key_stack.size() == 1) {  // this is the progenitor
    return;
  }
  // The current pending node becomes a prioritized non-blocking dependency of
  // its parent node; spread the pending state to its non-pending ancestors
  auto keys = key_stack;
  for (auto child = node_stack.rbegin(); child != node_stack.rend(); ++child) {
    auto last_key = keys.back();
    keys.pop_back();
    auto parent_path = ConfigData::JoinPath(keys);
    auto& parent_deps = deps[parent_path];
    bool parent_was_pending = !parent_deps.empty();
    // Pending children should be resolved before applying __include or __patch
    InsertByPriority(parent_deps,
                     New<PendingChild>(parent_path + "/" + last_key, *child));
    DLOG(INFO) << "parent_path = " << parent_path
               << ", #deps = " << parent_deps.size();
    if (parent_was_pending ||  // so was all ancestors
        keys.size() == 1) {  // this parent is the progenitor
      return;
    }
  }
}

ConfigCompiler::ConfigCompiler(ResourceResolver* resource_resolver)
    : resource_resolver_(resource_resolver),
      graph_(new ConfigDependencyGraph) {
}

ConfigCompiler::~ConfigCompiler() {
}

Reference ConfigCompiler::CreateReference(const string& qualified_path) {
  auto end = qualified_path.find_last_of("?");
  bool optional = end != string::npos;
  auto separator = qualified_path.find_first_of(":");
  string resource_id = resource_resolver_->ToResourceId(
      (separator == string::npos || separator == 0) ?
      graph_->current_resource_id() :
      qualified_path.substr(0, separator));
  string local_path = (separator == string::npos) ?
      qualified_path.substr(0, end) :
      qualified_path.substr(separator + 1,
                            optional ? end - separator - 1 : end);
  return Reference{resource_id, local_path, optional};
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
  resource->loaded = resource->data->LoadFromFile(
      resource_resolver_->ResolvePath(resource_id).string(), this);
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
  DLOG(INFO) << "blocking node: " << path;
  if (compiler->ResolveDependencies(path)) {
    DLOG(INFO) << "resolved blocking node:" << path;
    return true;
  }
  return false;
}

static an<ConfigItem> GetResolvedItem(ConfigCompiler* compiler,
                                      an<ConfigResource> resource,
                                      const string& path) {
  DLOG(INFO) << "GetResolvedItem(" << resource->resource_id << ":" << path << ")";
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
      DLOG(INFO) << "advance with key: " << key;
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
    DLOG(INFO) << "resource not loaded, compiling: " << reference.resource_id;
    resource = compiler->Compile(reference.resource_id);
    if (!resource->loaded) {
      if (reference.optional) {
        LOG(INFO) << "optional resource not loaded: " << reference.resource_id;
      } else {
        LOG(ERROR) << "resource could not be loaded: " << reference.resource_id;
      }
      return nullptr;
    }
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
    DLOG(INFO) << "ParseInclude(" << path << ")";
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
    DLOG(INFO) << "ParsePatch(" << path << ")";
    compiler->AddDependency(
        New<PatchReference>(compiler->CreateReference(path)));
    return true;
  }
  if (Is<ConfigMap>(item)) {
    DLOG(INFO) << "ParsePatch(<literal>)";
    compiler->AddDependency(New<PatchLiteral>(As<ConfigMap>(item)));
    return true;
  }
  return false;
}

bool ConfigCompiler::Parse(const string& key, const an<ConfigItem>& item) {
  DLOG(INFO) << "ConfigCompiler::Parse(" << key << ")";
  if (key == INCLUDE_DIRECTIVE) {
    return ParseInclude(this, item);
  }
  if (key == PATCH_DIRECTIVE) {
    return ParseList(ParsePatch, this, item);
  }
  return false;
}

bool ConfigCompiler::Link(an<ConfigResource> target) {
  DLOG(INFO) << "Link(" << target->resource_id << ")";
  auto found = graph_->resources.find(target->resource_id);
  if (found == graph_->resources.end()) {
    LOG(ERROR) << "resource not found: " << target->resource_id;
    return false;
  }
  return ResolveDependencies(found->first + ":");
}

bool ConfigCompiler::ResolveDependencies(const string& path) {
  DLOG(INFO) << "ResolveDependencies(" << path << ")";
  if (!graph_->deps.count(path)) {
    return true;
  }
  auto& deps = graph_->deps[path];
  for (auto iter = deps.begin(); iter != deps.end(); ) {
    if (!(*iter)->Resolve(this)) {
      LOG(ERROR) << "unresolved dependency: " << **iter;
      return false;
    }
    LOG(INFO) << "resolved: " << **iter;
    iter = deps.erase(iter);
  }
  DLOG(INFO) << "all dependencies resolved.";
  return true;
}

}  // namespace rime
