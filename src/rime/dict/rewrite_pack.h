//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_PACK_H_
#define RIME_REWRITE_PACK_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

struct RewriteEntry {
  string key;
  string value;
};

struct RewriteStageData {
  vector<RewriteEntry> entries;
};

struct RewriteSectionData {
  string name;
  // Exactly one stage belongs to a section. Pipeline order is supplied by
  // engine/filters, not encoded as a conversion chain here.
  RewriteStageData stage;
};

class RewriteSection;

class RIME_DLL RewritePack {
 public:
  // Non-owning, read-only stage pointer. Keep this pack alive while using it.
  // Resolve once at filter initialization; queries do not revisit the index.
  using StageHandle = const RewriteSection*;

  ~RewritePack();

  static std::shared_ptr<RewritePack> OpenCached(const path& file_path);

  StageHandle FindSection(const string& name) const;
  const path& file_path() const { return file_path_; }
  uint64_t build_id() const { return build_id_; }

 private:
  friend class RewritePackBuilder;
  class Impl;

  explicit RewritePack(const path& file_path);
  bool Open();

  path file_path_;
  uint64_t build_id_ = 0;
  std::unique_ptr<Impl> impl_;
};

class RIME_DLL RewriteSection {
 public:
  ~RewriteSection();

  // Read only this stage's mapped trie and values, with a query-local Agent.
  bool Exact(const string& text, vector<string>* values) const;
  bool ConvertSentence(const string& text, string* result) const;

 private:
  friend class RewritePack;
  class Impl;

  RewriteSection();
  std::unique_ptr<Impl> impl_;
};

class RIME_DLL RewritePackBuilder {
 public:
  bool Build(const path& output_path,
             const vector<RewriteSectionData>& sections) const;
};

}  // namespace rime

#endif  // RIME_REWRITE_PACK_H_
