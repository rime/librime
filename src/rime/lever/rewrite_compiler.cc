//
// Copyright RIME Developers
// Distributed under the BSD License
//
// The deployment compiler deliberately understands only the native files
// contract. Format adapters (if any) must produce these files before this
// component is invoked; the compiler itself has no knowledge of their source
// format.
#include <rime/lever/rewrite_compiler.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <unordered_set>
#include <utility>

#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/schema.h>
#include <rime/dict/rewrite_pack.h>

namespace rime {
namespace {

namespace fs = std::filesystem;
constexpr size_t kMaxSourceLineBytes = 16 * 1024 * 1024;

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
  if (line->size() >= 3 &&
      static_cast<unsigned char>((*line)[0]) == 0xef &&
      static_cast<unsigned char>((*line)[1]) == 0xbb &&
      static_cast<unsigned char>((*line)[2]) == 0xbf) {
    line->erase(0, 3);
  }

  const size_t first = line->find_first_not_of(" \t");
  return first != string::npos && (*line)[first] != '#';
}

string DecodeEscapes(const string& text) {
  string result;
  result.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 >= text.size()) {
      result.push_back(text[i]);
      continue;
    }
    switch (text[++i]) {
      case 'n':
        result.push_back('\n');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case '\\':
        result.push_back('\\');
        break;
      default:
        // Unknown escapes are kept losslessly instead of silently changing
        // user data.
        result.push_back('\\');
        result.push_back(text[i]);
        break;
    }
  }
  return result;
}

bool ParseFileLine(const string& source,
                   string* key,
                   vector<string>* values) {
  if (!key || !values) {
    return false;
  }

  const size_t tab = source.find('\t');
  if (tab == string::npos || tab == 0 || tab + 1 >= source.size()) {
    return false;
  }

  *key = DecodeEscapes(source.substr(0, tab));
  const string value = DecodeEscapes(source.substr(tab + 1));
  values->clear();
  if (key->empty() || value.empty()) {
    return false;
  }
  values->push_back(value);
  return true;
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

path ResolveDataFile(Deployer* deployer, const string& file_name) {
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
    LOG(ERROR) << "rewrite source must be relative to a data directory: "
               << file_name;
    return path();
  }
  // Inspect the spelling supplied by the schema before normalizing it. A
  // path such as "a/../b" happens to remain under the root, but accepting
  // parent components makes the boundary harder to audit and broadens the
  // file contract unnecessarily.
  for (const auto& component : requested) {
    if (component == "..") {
      LOG(ERROR) << "rewrite source escapes its data directory: " << file_name;
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
  return path();
}

bool LoadSourceFile(const path& file_path, RewriteStageData* stage) {
  if (!stage) {
    return false;
  }

  std::ifstream in(file_path, std::ios::binary);
  if (!in) {
    LOG(ERROR) << "failed to open rewrite source: " << file_path;
    return false;
  }

  string line;
  size_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    if (line.size() > kMaxSourceLineBytes) {
      LOG(ERROR) << "rewrite source line is too long " << file_path << ':'
                 << line_number;
      return false;
    }
    if (!PrepareSourceLine(&line)) {
      continue;
    }

    string key;
    vector<string> values;
    if (!ParseFileLine(line, &key, &values)) {
      LOG(ERROR) << "malformed rewrite line " << file_path << ':'
                 << line_number << "; expected key<TAB>value";
      return false;
    }
    for (const auto& value : values) {
      stage->entries.push_back({key, value});
    }
  }

  if (in.bad()) {
    LOG(ERROR) << "failed while reading rewrite source: " << file_path;
    return false;
  }
  return true;
}

void ReadStringList(Config* config,
                    const string& key,
                    vector<string>* values) {
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
  std::unordered_set<string> seen;
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

bool CompileSection(Config* config,
                    Deployer* deployer,
                    const string& section_name,
                    RewriteSectionData* output) {
  if (!config || !deployer || !output) {
    return false;
  }

  output->name = section_name;
  output->stage.entries.clear();

  vector<string> files;
  ReadStringList(config, section_name + "/files", &files);
  if (files.empty()) {
    LOG(ERROR) << "rewriter@" << section_name
               << " requires a non-empty 'files' list.";
    return false;
  }

  // One rewriter section is one pipeline stage. All files listed here are
  // merged into this single stage/trie in the declared order.
  std::unordered_set<string> seen_files;
  seen_files.reserve(files.size());
  for (const auto& file_name : files) {
    const path file_path = ResolveDataFile(deployer, file_name);
    if (file_path.empty()) {
      LOG(ERROR) << "rewrite source not found or outside data roots: "
                 << file_name;
      return false;
    }

    const string identity = file_path.generic_string();
    if (!seen_files.insert(identity).second) {
      LOG(WARNING) << "rewrite source listed more than once; skipped: "
                   << file_name;
      continue;
    }
    if (!LoadSourceFile(file_path, &output->stage)) {
      return false;
    }
  }

  if (output->stage.entries.empty()) {
    LOG(ERROR) << "rewriter@" << section_name
               << " has no usable entries in its files.";
    return false;
  }
  return true;
}

bool RemoveStalePack(const path& output_path) {
  std::error_code ec;
  const fs::file_status status = fs::symlink_status(output_path, ec);
  if (ec == std::errc::no_such_file_or_directory) {
    return true;
  }
  if (ec) {
    LOG(ERROR) << "failed to inspect stale rewrite pack '" << output_path
               << "': " << ec.message();
    return false;
  }
  if (!fs::is_regular_file(status)) {
    LOG(ERROR) << "refusing to remove non-regular rewrite pack path '"
               << output_path << "'.";
    return false;
  }
  if (!fs::remove(output_path, ec) && ec) {
    LOG(ERROR) << "failed to remove stale rewrite pack '" << output_path
               << "': " << ec.message();
    return false;
  }
  return true;
}

}  // namespace

RewriteCompiler::RewriteCompiler(Schema* schema, Deployer* deployer)
    : schema_(schema), deployer_(deployer) {}

bool RewriteCompiler::Compile() {
  if (!schema_ || !deployer_ || !schema_->config()) {
    return false;
  }

  Config* config = schema_->config();
  const path schema_id_path(schema_->schema_id());
  if (schema_->schema_id().empty() || schema_id_path.is_absolute() ||
      schema_id_path.has_parent_path() ||
      schema_id_path.filename().string() != schema_->schema_id()) {
    LOG(ERROR) << "invalid schema id for rewrite pack: "
               << schema_->schema_id();
    return false;
  }
  const vector<string> section_names = FindRewriterSections(config);
  const path output_path =
      deployer_->staging_dir / (schema_->schema_id() + ".rwp");

  if (section_names.empty()) {
    // Do not leave a previous schema pack in the staging tree after the last
    // rewriter filter is removed.
    return RemoveStalePack(output_path);
  }

  vector<RewriteSectionData> sections;
  sections.reserve(section_names.size());
  for (const auto& section_name : section_names) {
    RewriteSectionData section;
    if (!CompileSection(config, deployer_, section_name, &section)) {
      return false;
    }
    sections.push_back(std::move(section));
  }

  LOG(INFO) << "compiling rewrite pack '" << output_path << "'.";
  RewritePackBuilder builder;
  if (!builder.Build(output_path, sections)) {
    LOG(ERROR) << "failed to compile rewrite pack '" << output_path << "'.";
    return false;
  }
  LOG(INFO) << "rewrite pack is ready: " << output_path;
  return true;
}

}  // namespace rime
