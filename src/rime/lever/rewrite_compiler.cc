//
// Copyright RIME Developers
// Distributed under the BSD License
//
// The deployment compiler accepts either native files or a shared preset.
// A preset expands to independent reusable stages; execution order is not
// encoded in RewriteStore/RewritePack and remains a runtime concern.
#include <rime/lever/rewrite_compiler.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>

#include <rime/algo/algebra.h>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/lever/rewrite_preset.h>
#include <rime/dict/rewrite_store.h>

namespace rime {

namespace {

namespace fs = std::filesystem;
constexpr size_t kMaxSourceLineBytes = 16 * 1024 * 1024;
constexpr uint64_t kFnvOffsetBasis = 1469598103934665603ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;
constexpr uint64_t kStageFingerprintContractVersion = 1;
constexpr uint64_t kSecondHashSeed = 0x9e3779b97f4a7c15ULL;

bool StartsWith(const string& text, const string& prefix) {
  return text.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), text.begin());
}

bool PrepareSourceLine(string* line) {
  if (!line) {
    return false;
  }
  if (!line->empty() && line->back() == '\r') {
    line->pop_back();
  }
  if (line->size() >= 3 && static_cast<unsigned char>((*line)[0]) == 0xef &&
      static_cast<unsigned char>((*line)[1]) == 0xbb &&
      static_cast<unsigned char>((*line)[2]) == 0xbf) {
    line->erase(0, 3);
  }

  const size_t first = line->find_first_not_of(" \t");
  return first != string::npos && (*line)[first] != '#';
}

bool ParseFileLine(std::string_view source, string* key, string* value) {
  if (!key || !value) {
    return false;
  }

  // The source format is literal key<TAB>value. Only the first real tab is
  // structural; everything after it belongs to value unchanged.
  const size_t tab = source.find('\t');
  if (tab == std::string_view::npos || tab == 0 || tab + 1 >= source.size()) {
    return false;
  }

  key->assign(source.substr(0, tab));
  value->assign(source.substr(tab + 1));
  return !key->empty() && !value->empty();
}

bool IsWithin(const path& root, const path& candidate) {
  auto root_it = root.begin();
  auto candidate_it = candidate.begin();
  for (; root_it != root.end() && candidate_it != candidate.end();
       ++root_it, ++candidate_it) {
    if (*root_it != *candidate_it) {
      return false;
    }
  }
  return root_it == root.end();
}

path ResolveUnderRoot(const path& root, const path& relative) {
  if (root.empty()) {
    return path();
  }
  std::error_code ec;
  const path canonical_root = fs::weakly_canonical(root, ec);
  if (ec || !fs::is_directory(canonical_root, ec) || ec) {
    return path();
  }

  const path candidate = canonical_root / relative;
  const path canonical_candidate = fs::canonical(candidate, ec);
  if (ec || !fs::is_regular_file(canonical_candidate, ec) || ec ||
      !IsWithin(canonical_root, canonical_candidate)) {
    return path();
  }
  return canonical_candidate;
}

path ResolveDataFile(Deployer* deployer,
                     const string& section_name,
                     const string& file_name) {
  if (!deployer || file_name.empty()) {
    return path();
  }

  const path requested(file_name);
  // Source files are data resources, not arbitrary paths. Reject absolute
  // paths and parent traversal so a schema cannot make deployment read a
  // file outside the configured data roots. Symlink escapes are rejected by
  // ResolveUnderRoot as well.
  if (requested.is_absolute() || requested.has_root_name() ||
      requested.has_root_directory()) {
    LOG(ERROR)
        << "rewriter@" << section_name << "/files contains absolute path '"
        << file_name
        << "'; use a path relative to Rime's user/shared data directory.";
    return path();
  }
  // Inspect the spelling supplied by the schema before normalizing it. A
  // path such as "a/../b" happens to remain under the root, but accepting
  // parent components makes the boundary harder to audit and broadens the
  // file contract unnecessarily.
  for (const auto& component : requested) {
    if (component == "..") {
      LOG(ERROR) << "rewriter@" << section_name
                 << "/files contains unsafe path '" << file_name
                 << "'; '..' components are not allowed.";
      return path();
    }
  }
  const path relative = requested.lexically_normal();

  path resolved = ResolveUnderRoot(deployer->user_data_dir, relative);
  if (!resolved.empty()) {
    return resolved;
  }
  resolved = ResolveUnderRoot(deployer->shared_data_dir, relative);
  if (!resolved.empty()) {
    return resolved;
  }
  LOG(ERROR) << "rewriter@" << section_name << " source '" << file_name
             << "' was not found as a regular file inside Rime data "
                "directories; searched user='"
             << deployer->user_data_dir << "', shared='"
             << deployer->shared_data_dir << "'. Check rewriter@"
             << section_name
             << "/files and the file path; symlinks must remain inside these "
                "directories.";
  return path();
}

void ReadStringList(Config* config, const string& key, vector<string>* values);

struct RewriteSourceFile {
  string declared_name;
  path resolved_path;
};

struct RewriteSourceSection {
  string name;
  string key_xlit;
  vector<RewriteSourceFile> files;
};

struct RewriteSourceBinding {
  string name;
  vector<RewriteSourceSection> stages;
};

bool ResolveRewriteSources(Config* config,
                           Deployer* deployer,
                           const vector<string>& section_names,
                           vector<RewriteSourceBinding>* bindings) {
  if (!config || !deployer || !bindings) {
    return false;
  }

  bindings->clear();
  bindings->reserve(section_names.size());
  std::unique_ptr<RewritePresetCatalog> presets;
  for (const auto& section_name : section_names) {
    const bool has_files =
        static_cast<bool>(config->GetItem(section_name + "/files"));
    const bool has_preset =
        static_cast<bool>(config->GetItem(section_name + "/preset"));
    if (has_files == has_preset) {
      LOG(ERROR) << "rewriter@" << section_name
                 << " must configure exactly one of /files or /preset.";
      return false;
    }

    RewriteSourceBinding binding;
    binding.name = section_name;
    if (has_files) {
      vector<string> files;
      ReadStringList(config, section_name + "/files", &files);
      if (files.empty()) {
        LOG(ERROR) << "rewriter@" << section_name << "/files is empty.";
        return false;
      }

      RewriteSourceSection stage;
      stage.name = section_name;
      config->GetString(section_name + "/key_xlit", &stage.key_xlit);
      stage.files.reserve(files.size());
      for (const auto& file_name : files) {
        const path file_path =
            ResolveDataFile(deployer, section_name, file_name);
        if (file_path.empty()) {
          return false;
        }
        stage.files.push_back({file_name, file_path});
      }
      binding.stages.push_back(std::move(stage));
    } else {
      string preset_name;
      if (!config->GetString(section_name + "/preset", &preset_name) ||
          preset_name.empty()) {
        LOG(ERROR) << "rewriter@" << section_name
                   << "/preset must be a non-empty preset name.";
        return false;
      }

      string key_xlit;
      if (config->GetString(section_name + "/key_xlit", &key_xlit) &&
          !key_xlit.empty()) {
        LOG(ERROR) << "rewriter@" << section_name
                   << " cannot combine /preset with /key_xlit.";
        return false;
      }

      if (!presets) {
        presets = std::make_unique<RewritePresetCatalog>(
            deployer->shared_data_dir / "opencc");
        if (!presets->Load()) {
          return false;
        }
      }
      vector<RewritePresetStage> preset_stages;
      if (!presets->Resolve(preset_name, &preset_stages)) {
        LOG(ERROR) << "rewriter@" << section_name << " cannot resolve preset '"
                   << preset_name << "'.";
        return false;
      }
      binding.stages.reserve(preset_stages.size());
      for (const auto& preset_stage : preset_stages) {
        RewriteSourceSection stage;
        stage.name = preset_stage.name;
        stage.files.reserve(preset_stage.files.size());
        for (const auto& file_path : preset_stage.files) {
          stage.files.push_back({file_path.filename().string(), file_path});
        }
        binding.stages.push_back(std::move(stage));
      }
    }

    if (binding.stages.empty()) {
      return false;
    }
    bindings->push_back(std::move(binding));
  }
  return true;
}

struct StageDescriptor {
  string key_xlit;
  vector<path> files;

  bool operator==(const StageDescriptor& other) const {
    return key_xlit == other.key_xlit && files == other.files;
  }
};

StageDescriptor DescribeStage(const RewriteSourceSection& source) {
  StageDescriptor descriptor;
  descriptor.key_xlit = source.key_xlit;
  descriptor.files.reserve(source.files.size());
  // Stage file lists are small and order is semantic. Linear dedup avoids
  // requiring a hash specialization for rime::path while preserving order.
  for (const auto& file : source.files) {
    if (std::find(descriptor.files.begin(), descriptor.files.end(),
                  file.resolved_path) == descriptor.files.end()) {
      descriptor.files.push_back(file.resolved_path);
    }
  }
  return descriptor;
}

void HashBytes(RewriteStageId* id, const char* data, size_t size) {
  if (!id) {
    return;
  }
  for (size_t i = 0; i < size; ++i) {
    const uint64_t byte = static_cast<unsigned char>(data[i]);
    id->high ^= byte;
    id->high *= kFnvPrime;
    id->low ^= byte + kSecondHashSeed + (id->low << 6) + (id->low >> 2);
    id->low *= 0x9ddfea08eb382d69ULL;
  }
}

void HashU64(RewriteStageId* id, uint64_t value) {
  char bytes[8];
  for (size_t i = 0; i < sizeof(bytes); ++i) {
    bytes[i] = static_cast<char>((value >> (i * 8)) & 0xff);
  }
  HashBytes(id, bytes, sizeof(bytes));
}

void HashString(RewriteStageId* id, const string& value) {
  HashU64(id, value.size());
  HashBytes(id, value.data(), value.size());
}

bool ComputeStageId(const RewriteSourceSection& source,
                    RewriteStageId* stage_id) {
  if (!stage_id) {
    return false;
  }

  RewriteStageId id{kFnvOffsetBasis, kSecondHashSeed};
  HashU64(&id, kStageFingerprintContractVersion);
  HashString(&id, source.key_xlit);

  vector<path> unique_files;
  unique_files.reserve(source.files.size());
  for (const auto& file : source.files) {
    if (std::find(unique_files.begin(), unique_files.end(),
                  file.resolved_path) == unique_files.end()) {
      unique_files.push_back(file.resolved_path);
    }
  }
  HashU64(&id, unique_files.size());

  for (const auto& file_path : unique_files) {
    std::error_code ec;
    const uint64_t file_size = fs::file_size(file_path, ec);
    if (ec) {
      LOG(ERROR) << "rewriter@" << source.name
                 << " cannot read metadata for source '" << file_path
                 << "': " << ec.message()
                 << "; check that the file still exists and is readable.";
      return false;
    }

    // Stage identity follows ordered source bytes and key_xlit semantics.
    HashU64(&id, file_size);
    std::ifstream in(static_cast<const std::filesystem::path&>(file_path),
                     std::ios::binary);
    if (!in) {
      LOG(ERROR) << "rewriter@" << source.name << " cannot open source '"
                 << file_path << "' while computing its shared-stage id.";
      return false;
    }

    std::array<char, 64 * 1024> buffer{};
    while (in) {
      in.read(buffer.data(), buffer.size());
      const std::streamsize size = in.gcount();
      if (size > 0) {
        HashBytes(&id, buffer.data(), static_cast<size_t>(size));
      }
    }
    if (!in.eof()) {
      LOG(ERROR) << "rewriter@" << source.name
                 << " encountered an I/O error while hashing source '"
                 << file_path << "'.";
      return false;
    }
  }

  if (id.empty()) {
    id.low = 1;
  }
  *stage_id = id;
  return true;
}

bool BuildKeyXlit(const string& section_name,
                  const string& key_xlit,
                  Projection* key_xlit_projection) {
  if (!key_xlit_projection || key_xlit.empty()) {
    return false;
  }

  const size_t separator = key_xlit.find('>');
  if (separator == string::npos || separator == 0 ||
      separator + 1 >= key_xlit.size() ||
      key_xlit.find('>', separator + 1) != string::npos) {
    LOG(ERROR) << "rewriter@" << section_name << "/key_xlit='" << key_xlit
               << "' is invalid; expected exactly one non-empty 'from>to' "
                  "mapping, for example 'jys>597'.";
    return false;
  }

  const string from = key_xlit.substr(0, separator);
  const string to = key_xlit.substr(separator + 1);
  const string formula = "xlit/" + from + "/" + to + "/";
  auto rules = New<ConfigList>();
  rules->Append(New<ConfigValue>(formula));
  if (!key_xlit_projection->Load(rules)) {
    LOG(ERROR) << "rewriter@" << section_name << "/key_xlit='" << key_xlit
               << "' could not be compiled as xlit; verify that both sides "
                  "form a valid transliteration mapping.";
    return false;
  }
  return true;
}

bool LoadSourceFile(const string& section_name,
                    const path& file_path,
                    Projection* key_xlit_projection,
                    RewriteStageData* stage) {
  if (!stage) {
    return false;
  }

  std::ifstream in(static_cast<const std::filesystem::path&>(file_path),
                   std::ios::binary);
  if (!in) {
    LOG(ERROR) << "rewriter@" << section_name << " cannot open source '"
               << file_path << "'; check that the file exists and is readable.";
    return false;
  }

  string line;
  size_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    if (line.size() > kMaxSourceLineBytes) {
      LOG(ERROR) << "rewriter@" << section_name << " source " << file_path
                 << ':' << line_number
                 << " exceeds the 16 MiB per-line limit; split or shorten "
                    "this entry.";
      return false;
    }
    if (!PrepareSourceLine(&line)) {
      continue;
    }

    string key;
    string value;
    if (!ParseFileLine(line, &key, &value)) {
      LOG(ERROR) << "rewriter@" << section_name << " source " << file_path
                 << ':' << line_number
                 << " is malformed; expected non-empty key<TAB>value.";
      return false;
    }

    string preedit;
    if (key_xlit_projection) {
      const string source_key = key;
      key_xlit_projection->Apply(&key);
      if (key.empty()) {
        LOG(ERROR) << "rewriter@" << section_name
                   << "/key_xlit converted source key '" << source_key
                   << "' to an empty key at " << file_path << ':' << line_number
                   << "; adjust key_xlit so every source key remains "
                      "non-empty.";
        return false;
      }
      if (key != source_key) {
        preedit = source_key;
      }
    }

    stage->entries.push_back(
        {std::move(key), std::move(value), std::move(preedit)});
  }

  if (in.bad()) {
    LOG(ERROR) << "rewriter@" << section_name
               << " encountered an I/O error while reading source '"
               << file_path << "'; check file permissions and storage health.";
    return false;
  }
  return true;
}

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

vector<string> FindRewriterSections(Config* config) {
  vector<string> result;
  hash_set<string> seen;
  auto filters = config->GetList("engine/filters");
  if (!filters) {
    return result;
  }

  for (auto it = filters->begin(); it != filters->end(); ++it) {
    auto value = As<ConfigValue>(*it);
    if (!value) {
      continue;
    }

    const string& component = value->str();
    string section;
    if (component == "rewriter") {
      section = "rewriter";
    } else if (StartsWith(component, "rewriter@")) {
      section = component.substr(sizeof("rewriter@") - 1);
    } else {
      continue;
    }

    if (!section.empty() && seen.insert(section).second) {
      result.push_back(std::move(section));
    }
  }
  return result;
}

bool CompileStage(const RewriteSourceSection& source,
                  RewriteStageData* output) {
  if (!output) {
    return false;
  }

  output->entries.clear();

  Projection key_xlit_projection;
  Projection* active_key_xlit = nullptr;
  if (!source.key_xlit.empty()) {
    if (!BuildKeyXlit(source.name, source.key_xlit, &key_xlit_projection)) {
      return false;
    }
    active_key_xlit = &key_xlit_projection;
  }

  // Files listed for one source stage are merged in declared order.
  vector<path> seen_files;
  seen_files.reserve(source.files.size());
  for (const auto& file : source.files) {
    const string& file_name = file.declared_name;
    const path& file_path = file.resolved_path;
    if (std::find(seen_files.begin(), seen_files.end(), file_path) !=
        seen_files.end()) {
      LOG(WARNING) << "rewriter@" << source.name << " lists source '"
                   << file_name
                   << "' more than once; later duplicate is ignored.";
      continue;
    }
    seen_files.push_back(file_path);
    if (!LoadSourceFile(source.name, file_path, active_key_xlit, output)) {
      return false;
    }
  }

  if (output->entries.empty()) {
    LOG(ERROR) << "rewriter@" << source.name
               << " produced no usable entries; check its files for "
                  "non-empty key<TAB>value lines.";
    return false;
  }
  return true;
}

}  // namespace

RewriteCompiler::RewriteCompiler(const string& schema_id,
                                 Config* config,
                                 Deployer* deployer)
    : schema_id_(schema_id), config_(config), deployer_(deployer) {}

bool RewriteCompiler::Compile() {
  if (!config_ || !deployer_) {
    return false;
  }

  const path schema_id_path(schema_id_);
  if (schema_id_.empty() || schema_id_path.is_absolute() ||
      schema_id_path.has_parent_path() ||
      schema_id_path.filename().string() != schema_id_) {
    LOG(ERROR) << "cannot update shared rewrite store: schema id '"
               << schema_id_
               << "' is invalid; expected a single non-empty schema id "
                  "without path components.";
    return false;
  }

  std::error_code ec;
  fs::create_directories(deployer_->staging_dir, ec);
  if (ec) {
    LOG(ERROR) << "cannot create rewrite staging directory '"
               << deployer_->staging_dir << "': " << ec.message()
               << "; check directory permissions and available storage.";
    return false;
  }

  const path store_path = deployer_->staging_dir / "rewriter.rwp";
  RewriteStoreWriter store(store_path);
  if (!store.Open()) {
    return false;
  }

  const vector<string> section_names = FindRewriterSections(config_);
  if (section_names.empty()) {
    return store.CommitSchema(schema_id_, {});
  }

  vector<RewriteSourceBinding> sources;
  if (!ResolveRewriteSources(config_, deployer_, section_names, &sources)) {
    return false;
  }

  struct CachedStage {
    StageDescriptor descriptor;
    RewriteStageId id;
  };
  vector<CachedStage> stage_cache;
  vector<RewriteStageBinding> bindings;
  bindings.reserve(sources.size());

  for (const auto& source_binding : sources) {
    RewriteStageBinding binding;
    binding.name = source_binding.name;
    binding.stages.reserve(source_binding.stages.size());

    for (const auto& source : source_binding.stages) {
      const StageDescriptor descriptor = DescribeStage(source);
      RewriteStageId stage_id;
      const auto cached = std::find_if(
          stage_cache.begin(), stage_cache.end(),
          [&](const auto& item) { return item.descriptor == descriptor; });
      if (cached != stage_cache.end()) {
        stage_id = cached->id;
      } else {
        if (!ComputeStageId(source, &stage_id)) {
          return false;
        }
        stage_cache.push_back({descriptor, stage_id});
        if (!store.HasStage(stage_id)) {
          RewriteStageData stage;
          if (!CompileStage(source, &stage) ||
              !store.AppendStage(stage_id, std::move(stage))) {
            LOG(ERROR) << "failed to compile rewrite stage '" << source.name
                       << "'.";
            return false;
          }
        }
      }
      binding.stages.push_back(stage_id);
    }
    bindings.push_back(std::move(binding));
  }

  if (!store.CommitSchema(schema_id_, bindings)) {
    LOG(ERROR) << "failed to commit rewrite index for schema '" << schema_id_
               << "'.";
    return false;
  }
  return true;
}

bool RewriteCompiler::BeginWorkspace(Deployer* deployer) {
  if (!deployer) {
    return false;
  }
  RewriteStoreWriter store(deployer->staging_dir / "rewriter.rwp");
  return store.BeginWorkspace();
}

bool RewriteCompiler::AbortWorkspace(Deployer* deployer) {
  if (!deployer) {
    return false;
  }
  RewriteStoreWriter store(deployer->staging_dir / "rewriter.rwp");
  return store.AbortWorkspace();
}

bool RewriteCompiler::FinalizeWorkspace(const vector<string>& schema_ids,
                                        Deployer* deployer) {
  if (!deployer) {
    return false;
  }
  const path store_path = deployer->staging_dir / "rewriter.rwp";
  RewriteStoreWriter store(store_path);
  if (!store.Open() || !store.RetainSchemas(schema_ids)) {
    return false;
  }
  return store.CommitWorkspace();
}

}  // namespace rime
