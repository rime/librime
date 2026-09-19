//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITER_H_
#define RIME_REWRITER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <rime/filter.h>
#include <rime/dict/rewrite_pack.h>
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
  bool AppliesToSegment(Segment* segment) override {
    return TagsMatch(segment);
  }

 private:
  friend class RewriterTranslation;
  friend class RewriterAbbrevTranslation;

  enum class Mode { kAppend, kReplace, kComment, kAbbrev };
  enum class Match { kExact, kSentence, kAuto };
  enum class CommentMode { kNone, kText, kComment };

  struct Options {
    bool always_on = false;
    vector<string> option_names;
    Mode mode = Mode::kAppend;
    Match match = Match::kExact;
    CommentMode comment_mode = CommentMode::kNone;
    string comment_format = "〔%s〕";
    string cand_type;
    set<string> excluded_types;
    size_t position = 1;
    size_t count = 1;
  };

  void LoadConfig();
  bool Active() const;
  bool Rewrite(const string& text, vector<string>* values) const;
  bool Transform(const an<Candidate>& candidate, CandidateQueue* result) const;
  string CandidateType(const an<Candidate>& candidate) const;
  string DerivedComment(const an<Candidate>& candidate) const;
  string FormatComment(const string& value) const;

  Options options_;
  // Retain the mapping for stage_, including while a new pack is deployed.
  std::shared_ptr<const RewritePack> pack_;
  RewritePack::StageHandle stage_ = nullptr;
};

class RewriterComponent : public Rewriter::Component {
 public:
  Rewriter* Create(const Ticket& ticket) override {
    return new Rewriter(ticket);
  }
};

}  // namespace rime

#endif  // RIME_REWRITER_H_
