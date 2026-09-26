//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_PACK_H_
#define RIME_REWRITE_PACK_H_

#include <cstddef>
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

// Runtime-only hot state for enable_sentence. Only successful, reusable
// matches are cached; failed probes only advance a dependency barrier while
// rebuilding the uncertain tail.
struct RewriteSentenceHit {
  size_t input_end = 0;
  size_t output_end = 0;
  size_t required_prefix = 0;
  bool changed = false;
};

// Runtime-only continuation state for the earliest phrase probe whose trie
// path reached the current input end. On a pure append, the next call can
// continue from |node_pos| instead of traversing the already-seen suffix again.
struct RewriteSentenceFrontier {
  bool valid = false;
  size_t input_start = 0;
  size_t output_start = 0;
  size_t hit_count = 0;
  size_t dependency_barrier = 0;
  size_t first_char_size = 0;
  size_t suffix_bytes = 0;
  size_t node_pos = 0;
  uint32_t longest_value = 0;
  size_t longest_length = 0;
  bool changed_before = false;

  void Reset() { *this = RewriteSentenceFrontier{}; }
};

struct RewriteSentenceState {
  string input;
  string output;
  vector<RewriteSentenceHit> hits;
  RewriteSentenceFrontier frontier;
  bool changed = false;

  void Reset() {
    string().swap(input);
    string().swap(output);
    vector<RewriteSentenceHit>().swap(hits);
    frontier.Reset();
    changed = false;
  }

  size_t RetainedBytes() const {
    return input.capacity() + output.capacity() +
           hits.capacity() * sizeof(RewriteSentenceHit);
  }
};

// Optional per-call diagnostics for isolated sentence benchmarking.
// Rewriter passes nullptr in production, so normal candidate processing does
// not pay counter-maintenance cost.
struct RewriteSentenceMetrics {
  uint64_t calls = 0;
  uint64_t identical_input_hits = 0;
  uint64_t prefix_reuse_calls = 0;
  uint64_t full_rescan_calls = 0;
  uint64_t input_bytes = 0;
  uint64_t reused_input_bytes = 0;
  uint64_t rescanned_input_bytes = 0;
  uint64_t reused_hit_count = 0;
  uint64_t written_hit_count = 0;
  uint64_t phrase_probe_count = 0;
  uint64_t phrase_probe_restart_count = 0;
  uint64_t phrase_probe_continuation_count = 0;
  uint64_t phrase_probe_bytes = 0;
  uint64_t phrase_probe_reused_bytes = 0;
  uint64_t phrase_probe_mismatch_stops = 0;
  uint64_t phrase_probe_text_end_stops = 0;
  uint64_t unmatchable_skip_bytes = 0;
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
  bool ConvertSentenceIncremental(
      std::string_view text,
      RewriteSentenceState* state,
      RewriteSentenceMetrics* metrics = nullptr) const;

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
