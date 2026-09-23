//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITER_H_
#define RIME_REWRITER_H_

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <rime/filter.h>
#include <rime/dict/rewrite_store.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class RewriterTranslation;
class RewriterAbbrevTranslation;

class Rewriter : public Filter, public TagMatching {
 public:
  explicit Rewriter(const Ticket& ticket);
  ~Rewriter() override;

  an<Translation> Apply(an<Translation> translation,
                        CandidateList* candidates) override;
  bool AppliesToSegment(Segment* segment) override;

  // Maintenance-mode deployment drops all Rewriter-owned mappings while
  // keeping sessions alive. Rewriters reopen lazily after deployment.
  static void ReleaseStagesForDeployment();

 private:
  friend class RewriterTranslation;
  friend class RewriterAbbrevTranslation;

  enum class Mode { kDerive, kReplace, kComment, kAbbrev };
  enum class CommentSource { kNone, kText, kInherit };

  struct Options {
    bool always_on = false;
    vector<string> option_names;
    Mode mode = Mode::kDerive;
    bool enable_sentence = false;
    bool fallback_on_empty = true;
    CommentSource comment_source = CommentSource::kNone;
    string comment_template = "〔%s〕";
    string candidate_type;
    set<string> excluded_types;
    set<string> promote_on_types;
    size_t insert_position = 1;
    size_t insert_count = 1;
  };

  void LoadConfig();
  bool Active() const;
  struct LoadedStage {
    std::shared_ptr<const RewritePack> pack;
  };
  struct RuntimeState {
    std::shared_ptr<const RewriteStore> store;
    vector<LoadedStage> stages;
  };

  std::shared_ptr<const RuntimeState> EnsureState();
  bool Rewrite(const RuntimeState& state,
               std::string_view text,
               vector<string>* values,
               size_t candidate_rank,
               size_t segment_start,
               bool has_segment_range) const;
  bool RewriteWithPreedit(const RuntimeState& state,
                          std::string_view text,
                          vector<RewriteResult>* values) const;
  bool Transform(const RuntimeState& state,
                 const an<Candidate>& candidate,
                 CandidateQueue* result,
                 size_t candidate_rank,
                 size_t segment_start,
                 bool has_segment_range) const;
  const string& CandidateType(const an<Candidate>& candidate) const;
  string DerivedComment(const an<Candidate>& candidate) const;
  string ApplyCommentTemplate(const string& value) const;

  static constexpr size_t kSentenceCacheLanes = 8;
  static constexpr size_t kMaxSentenceCacheBytes = 64 * 1024;

  struct SentenceLane {
    bool valid = false;
    bool has_segment_range = false;
    size_t segment_start = 0;
    uint64_t store_generation = 0;
    vector<RewriteSentenceState> stages;

    void Reset() {
      valid = false;
      has_segment_range = false;
      segment_start = 0;
      store_generation = 0;
      for (auto& stage : stages) {
        stage.Reset();
      }
      vector<RewriteSentenceState>().swap(stages);
    }

    size_t RetainedBytes() const {
      size_t total = 0;
      for (const auto& stage : stages) {
        total += stage.RetainedBytes();
      }
      return total;
    }
  };

  void ClearSentenceCache();
  SentenceLane* PrepareSentenceLane(const RuntimeState& state,
                                    size_t candidate_rank,
                                    size_t segment_start,
                                    bool has_segment_range) const;

  Options options_;
  string schema_id_;
  path store_path_;
  std::shared_ptr<const RuntimeState> runtime_state_;
  std::mutex runtime_state_mutex_;
  bool runtime_state_load_attempted_ = false;

  mutable std::array<SentenceLane, kSentenceCacheLanes> sentence_lanes_;
  connection commit_connection_;
  connection abort_connection_;
  connection update_connection_;

  size_t segment_start_ = 0;
  size_t segment_end_ = 0;
  bool has_segment_range_ = false;
};

class RewriterComponent : public Rewriter::Component {
 public:
  Rewriter* Create(const Ticket& ticket) override {
    return new Rewriter(ticket);
  }
};

}  // namespace rime

#endif  // RIME_REWRITER_H_
