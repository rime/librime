//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_STORE_H_
#define RIME_REWRITE_STORE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <rime_api.h>
#include <rime/common.h>
#include <rime/dict/rewrite_pack.h>

namespace rime {

struct RewriteStageId {
  uint64_t high = 0;
  uint64_t low = 0;

  bool empty() const { return high == 0 && low == 0; }
  bool operator==(const RewriteStageId& other) const {
    return high == other.high && low == other.low;
  }
  bool operator!=(const RewriteStageId& other) const {
    return !(*this == other);
  }
};

// One rewriter section may bind to an ordered sequence of shared stages.
// Order and duplicates in |stages| are semantic.
struct RewriteStageBinding {
  string name;
  vector<RewriteStageId> stages;
};

class RIME_DLL RewriteStore {
 public:
  ~RewriteStore();

  static std::shared_ptr<RewriteStore> OpenCached(const path& file_path);

  bool FindStageSequence(const string& schema_id,
                         const string& section_name,
                         vector<RewriteStageId>* stage_ids) const;
  std::shared_ptr<const RewritePack> OpenStage(
      const RewriteStageId& stage_id) const;
  uint64_t generation() const { return generation_; }

 private:
  class Impl;

  explicit RewriteStore(const path& file_path);
  bool Open();

  path file_path_;
  uint64_t generation_ = 0;
  std::unique_ptr<Impl> impl_;
};

class RIME_DLL RewriteStoreWriter {
 public:
  explicit RewriteStoreWriter(const path& file_path);
  ~RewriteStoreWriter();

  bool Open();
  bool BeginWorkspace();
  bool AbortWorkspace();

  // Shared workspace failure propagation: once any schema compilation marks
  // the deployment failed, CommitWorkspace() must refuse a partial commit.
  void MarkWorkspaceFailed();
  bool WorkspaceFailed() const;

  bool HasStage(const RewriteStageId& stage_id) const;
  bool AppendStage(const RewriteStageId& stage_id, RewriteStageData stage);
  bool CommitSchema(const string& schema_id,
                    const vector<RewriteStageBinding>& bindings);
  bool RetainSchemas(const vector<string>& schema_ids);
  bool CommitWorkspace();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rime

#endif  // RIME_REWRITE_STORE_H_
