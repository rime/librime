//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/lever/rewrite_preset.h>

#include <filesystem>
#include <unordered_set>

#include <rime/config.h>

namespace rime {
namespace {

namespace fs = std::filesystem;

void ReadStringList(Config* config, const string& key, vector<string>* values) {
  values->clear();
  const auto item = config->GetItem(key);
  if (auto value = As<ConfigValue>(item)) {
    if (!value->str().empty()) {
      values->push_back(value->str());
    }
    return;
  }
  if (auto list = As<ConfigList>(item)) {
    for (size_t i = 0; i < list->size(); ++i) {
      if (auto value = As<ConfigValue>(list->GetAt(i))) {
        if (!value->str().empty()) {
          values->push_back(value->str());
        }
      }
    }
  }
}

bool IsSimpleFileName(const string& file_name) {
  if (file_name.empty()) {
    return false;
  }
  const path file_path(file_name);
  return !file_path.is_absolute() && !file_path.has_root_name() &&
         !file_path.has_root_directory() && !file_path.has_parent_path() &&
         file_path.filename().string() == file_name;
}

}  // namespace

class RewritePresetCatalog::Impl {
 public:
  explicit Impl(path root_value) : root_path(std::move(root_value)) {}

  path root_path;
  path canonical_root;
  the<Config> config;
};

RewritePresetCatalog::RewritePresetCatalog(const path& root_path)
    : impl_(new Impl(root_path)) {}

RewritePresetCatalog::~RewritePresetCatalog() = default;

bool RewritePresetCatalog::Load() {
  std::error_code ec;
  impl_->canonical_root = fs::weakly_canonical(impl_->root_path, ec);
  if (ec || !fs::is_directory(impl_->canonical_root, ec) || ec) {
    LOG(ERROR) << "rewrite preset directory '" << impl_->root_path
               << "' is not available.";
    return false;
  }

  const path preset_path = impl_->canonical_root / "presets.yaml";
  if (!fs::is_regular_file(preset_path, ec) || ec) {
    LOG(ERROR) << "rewrite preset file '" << preset_path << "' is missing.";
    return false;
  }

  impl_->config.reset(new Config);
  if (!impl_->config->LoadFromFile(preset_path)) {
    LOG(ERROR) << "cannot load rewrite preset file '" << preset_path << "'.";
    impl_->config.reset();
    return false;
  }
  int version = 0;
  if (!impl_->config->GetInt("version", &version) || version != 1) {
    LOG(ERROR) << "rewrite preset file '" << preset_path
               << "' has an unsupported version.";
    impl_->config.reset();
    return false;
  }
  return true;
}

bool RewritePresetCatalog::Resolve(const string& preset_name,
                                   vector<RewritePresetStage>* stages) const {
  if (!stages || !impl_->config || preset_name.empty()) {
    return false;
  }

  vector<string> stage_names;
  ReadStringList(impl_->config.get(), "presets/" + preset_name, &stage_names);
  if (stage_names.empty()) {
    LOG(ERROR) << "rewrite preset '" << preset_name << "' is not defined.";
    return false;
  }

  stages->clear();
  stages->reserve(stage_names.size());
  std::error_code ec;
  for (const auto& stage_name : stage_names) {
    if (stage_name.empty()) {
      return false;
    }

    vector<string> file_names;
    ReadStringList(impl_->config.get(), "stages/" + stage_name + "/files",
                   &file_names);
    if (file_names.empty()) {
      LOG(ERROR) << "rewrite preset stage '" << stage_name << "' has no files.";
      return false;
    }

    RewritePresetStage stage;
    stage.name = stage_name;
    stage.files.reserve(file_names.size());
    std::unordered_set<string> seen;
    for (const auto& file_name : file_names) {
      if (!IsSimpleFileName(file_name)) {
        LOG(ERROR) << "rewrite preset stage '" << stage_name
                   << "' contains invalid file name '" << file_name << "'.";
        return false;
      }
      if (!seen.insert(file_name).second) {
        continue;
      }

      const path file_path = impl_->canonical_root / file_name;
      const path canonical_file = fs::canonical(file_path, ec);
      if (ec || !fs::is_regular_file(canonical_file, ec) || ec ||
          canonical_file.parent_path() != impl_->canonical_root) {
        LOG(ERROR) << "rewrite preset source '" << file_path
                   << "' is not available.";
        return false;
      }
      stage.files.push_back(canonical_file);
    }
    stages->push_back(std::move(stage));
  }
  return true;
}

}  // namespace rime
