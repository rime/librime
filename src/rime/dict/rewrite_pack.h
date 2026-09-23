//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_PACK_H_
#define RIME_REWRITE_PACK_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

struct RewriteEntry {
  string key;
  string value;
  string preedit;
};

struct RewriteResult {
  string value;
  string preedit;
};

struct RewriteStageData {
  vector<RewriteEntry> entries;
};

class RewriteStore;

// One RewritePack is exactly one immutable, memory-mapped rewrite stage.
class RIME_DLL RewritePack {
 public:
  ~RewritePack();

  bool Lookup(std::string_view text, vector<string>* values) const;
  bool LookupWithPreedit(std::string_view text,
                         vector<RewriteResult>* values) const;
  bool ConvertSentence(std::string_view text, string* result) const;

 private:
  friend class RewriteStore;
  class Impl;

  RewritePack(const path& file_path, uint64_t offset, uint64_t size);
  bool Open();

  path file_path_;
  uint64_t offset_ = 0;
  uint64_t size_ = 0;
  std::unique_ptr<Impl> impl_;
};

class RIME_DLL RewritePackBuilder {
 public:
  bool Build(const path& output_path, const RewriteStageData& stage) const;
};

}  // namespace rime

#endif  // RIME_REWRITE_PACK_H_
