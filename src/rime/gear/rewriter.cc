//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/gear/rewriter.h>

#include <algorithm>
#include <deque>
#include <unordered_set>

#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/translation.h>
#include <rime/dict/rewrite_pack.h>

namespace rime {
namespace {

string Join(const vector<string>& values, const char* separator) {
  string result;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      result.append(separator);
    }
    result.append(values[i]);
  }
  return result;
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

path ResolvePackPath(const string& schema_id) {
  the<ResourceResolver> resolver(
      Service::instance().CreateDeployedResourceResolver(
          {"rewrite_pack", "", ".rwp"}));
  return resolver ? resolver->ResolvePath(schema_id) : path();
}

// A pure filter cannot tell which translator produced a candidate. These
// types are the observable signal used by the Lua implementation for the
// preferred/user stream: release all abbrev candidates before the first one.
// Other types keep the configured position/count policy.
bool IsAbbrevFrontedType(const an<Candidate>& candidate) {
  if (!candidate) {
    return false;
  }
  const string& type = candidate->type();
  return type == "table" || type == "user_table" || type == "completion";
}

}  // namespace

class RewriterTranslation : public PrefetchTranslation {
 public:
  RewriterTranslation(an<Translation> translation, const Rewriter* rewriter)
      : PrefetchTranslation(translation), rewriter_(rewriter) {}

 protected:
  bool Replenish() override {
    auto candidate = translation_->Peek();
    translation_->Next();
    if (!candidate) {
      return false;
    }
    if (!rewriter_->Transform(candidate, &cache_)) {
      cache_.push_back(candidate);
    }
    return !cache_.empty();
  }

 private:
  const Rewriter* rewriter_;
};

class RewriterAbbrevTranslation : public Translation {
 public:
  RewriterAbbrevTranslation(an<Translation> translation,
                            const Rewriter* rewriter,
                            Context* context)
      : translation_(translation), rewriter_(rewriter), context_(context) {
    BuildInjectedCandidates();
    PrepareNext();
  }

  an<Candidate> Peek() override { return current_; }

  bool Next() override {
    if (exhausted()) {
      return false;
    }
    if (current_is_injected_) {
      ++injected_index_;
      if (!release_all_ && injected_index_ >= front_limit_) {
        injected_index_ = injected_.size();
      }
    } else {
      ++regular_yielded_;
    }
    current_.reset();
    PrepareNext();
    return !exhausted();
  }

 private:
  void BuildInjectedCandidates() {
    if (!rewriter_->stage_ || !context_ || context_->input().empty()) {
      return;
    }

    size_t start = 0;
    size_t end = context_->input().size();
    if (!context_->composition().empty()) {
      const Segment& segment = context_->composition().back();
      start = segment.start;
      end = segment.end;
    }
    if (end <= start || end > context_->input().size()) {
      return;
    }

    const string input = context_->input().substr(start, end - start);
    vector<string> values;
    if (!rewriter_->stage_->Exact(input, &values)) {
      return;
    }
    std::unordered_set<string> seen;
    for (const auto& value : values) {
      if (value.empty() || !seen.insert(value).second) {
        continue;
      }
      const string type = rewriter_->options_.cand_type.empty()
                              ? "abbrev"
                              : rewriter_->options_.cand_type;
      auto candidate = New<SimpleCandidate>(type, start, end, value);
      injected_.push_back(candidate);
    }
    front_limit_ =
        std::min(rewriter_->options_.count, injected_.size());
  }

  void PrepareNext() {
    while (!current_) {
      // Peek once at the first visible candidate. The filter cannot inspect
      // the translator chain, so candidate type is the only reliable signal
      // available for Lua-compatible fallback placement.
      if (!first_candidate_checked_) {
        first_candidate_checked_ = true;
        if (translation_) {
          first_candidate_ = translation_->Peek();
          if (IsAbbrevFrontedType(first_candidate_)) {
            insertion_started_ = true;
            release_all_ = true;
            front_limit_ = injected_.size();
          }
        }
      }

      if (!injected_.empty() && injected_index_ < injected_.size()) {
        if (release_all_ || insertion_started_) {
          current_ = injected_[injected_index_];
          current_is_injected_ = true;
          break;
        }
        const size_t before = rewriter_->options_.position > 0
                                  ? rewriter_->options_.position - 1
                                  : 0;
        if (regular_yielded_ >= before) {
          insertion_started_ = true;
          current_ = injected_[injected_index_];
          current_is_injected_ = true;
          break;
        }
      }

      if (translation_ && (first_candidate_ || !translation_->exhausted())) {
        if (first_candidate_) {
          current_ = first_candidate_;
          first_candidate_.reset();
        } else {
          current_ = translation_->Peek();
        }
        translation_->Next();
        current_is_injected_ = false;
        if (current_) {
          break;
        }
        continue;
      }

      if (!insertion_started_ && !injected_.empty()) {
        insertion_started_ = true;
        // No original candidate was yielded. This is the same full-release
        // case as a priority first candidate, so no fallback switch is needed.
        if (regular_yielded_ == 0) {
          release_all_ = true;
          front_limit_ = injected_.size();
        }
        if (injected_index_ < front_limit_) {
          current_ = injected_[injected_index_];
          current_is_injected_ = true;
          break;
        }
      }

      set_exhausted(true);
      break;
    }
  }

  an<Translation> translation_;
  const Rewriter* rewriter_;
  Context* context_;
  vector<an<Candidate>> injected_;
  an<Candidate> current_;
  size_t injected_index_ = 0;
  size_t front_limit_ = 0;
  size_t regular_yielded_ = 0;
  an<Candidate> first_candidate_;
  bool first_candidate_checked_ = false;
  bool insertion_started_ = false;
  bool release_all_ = false;
  bool current_is_injected_ = false;
};

Rewriter::Rewriter(const Ticket& ticket)
    : Filter(ticket), TagMatching(ticket) {
  if (name_space_.empty() || name_space_ == "filter") {
    name_space_ = "rewriter";
  }
  LoadConfig();

  if (!engine_ || !engine_->schema()) {
    return;
  }
  const path pack_path = ResolvePackPath(engine_->schema()->schema_id());
  if (pack_path.empty()) {
    LOG(WARNING) << "rewrite pack not found for schema '"
                 << engine_->schema()->schema_id() << "'.";
    return;
  }
  pack_ = RewritePack::OpenCached(pack_path);
  if (!pack_) {
    return;
  }
  // Bind once; every subsequent query goes directly to this stage's view.
  stage_ = pack_->FindSection(name_space_);
  if (!stage_) {
    LOG(WARNING) << "rewrite section '" << name_space_ << "' not found in "
                 << pack_path;
  }
}

Rewriter::~Rewriter() = default;

void Rewriter::LoadConfig() {
  if (!engine_ || !engine_->schema() || !engine_->schema()->config()) {
    return;
  }
  Config* config = engine_->schema()->config();

  const auto option_item = config->GetItem(name_space_ + "/option");
  if (auto value = As<ConfigValue>(option_item)) {
    if (value->str() == "true") {
      options_.always_on = true;
    } else if (!value->str().empty() && value->str() != "false") {
      options_.option_names.push_back(value->str());
    }
  } else if (auto list = As<ConfigList>(option_item)) {
    for (size_t i = 0; i < list->size(); ++i) {
      if (auto value = As<ConfigValue>(list->GetAt(i))) {
        if (!value->str().empty()) {
          options_.option_names.push_back(value->str());
        }
      }
    }
  }

  string mode;
  config->GetString(name_space_ + "/mode", &mode);
  if (mode == "replace") {
    options_.mode = Mode::kReplace;
  } else if (mode == "comment") {
    options_.mode = Mode::kComment;
  } else if (mode == "abbrev") {
    options_.mode = Mode::kAbbrev;
  } else {
    options_.mode = Mode::kAppend;
  }

  string match;
  config->GetString(name_space_ + "/match", &match);
  if (match == "sentence") {
    options_.match = Match::kSentence;
  } else if (match == "auto") {
    options_.match = Match::kAuto;
  } else {
    options_.match = Match::kExact;
  }

  string comment_mode;
  config->GetString(name_space_ + "/comment_mode", &comment_mode);
  if (comment_mode == "text") {
    options_.comment_mode = CommentMode::kText;
  } else if (comment_mode == "comment") {
    options_.comment_mode = CommentMode::kComment;
  }
  config->GetString(name_space_ + "/comment_format",
                    &options_.comment_format);
  config->GetString(name_space_ + "/cand_type", &options_.cand_type);
  if (options_.cand_type == "preserve") {
    options_.cand_type.clear();
  }

  vector<string> excluded;
  ReadStringList(config, name_space_ + "/excluded_types", &excluded);
  options_.excluded_types.insert(excluded.begin(), excluded.end());

  int value = 0;
  if (config->GetInt(name_space_ + "/position", &value) && value > 0) {
    options_.position = static_cast<size_t>(value);
  }
  if (config->GetInt(name_space_ + "/count", &value) && value > 0) {
    options_.count = static_cast<size_t>(value);
  }
}

bool Rewriter::Active() const {
  if (options_.always_on) {
    return true;
  }
  if (!engine_ || !engine_->context()) {
    return false;
  }
  for (const auto& option_name : options_.option_names) {
    if (engine_->context()->get_option(option_name)) {
      return true;
    }
  }
  return false;
}

bool Rewriter::Rewrite(const string& text, vector<string>* values) const {
  values->clear();
  if (!stage_) {
    return false;
  }

  if (options_.match == Match::kExact) {
    if (!stage_->Exact(text, values)) {
      return false;
    }
  } else if (options_.match == Match::kSentence) {
    string rewritten;
    if (!stage_->ConvertSentence(text, &rewritten)) {
      return false;
    }
    values->push_back(std::move(rewritten));
  } else {
    if (!stage_->Exact(text, values)) {
      string rewritten;
      if (!stage_->ConvertSentence(text, &rewritten)) {
        return false;
      }
      values->push_back(std::move(rewritten));
    }
  }

  return !values->empty();
}

string Rewriter::CandidateType(const an<Candidate>& candidate) const {
  return options_.cand_type.empty() ? candidate->type() : options_.cand_type;
}

string Rewriter::FormatComment(const string& value) const {
  if (options_.comment_format.empty()) {
    return value;
  }
  string result = options_.comment_format;
  const size_t pos = result.find("%s");
  if (pos == string::npos) {
    return value;
  }
  result.replace(pos, 2, value);
  return result;
}

string Rewriter::DerivedComment(const an<Candidate>& candidate) const {
  switch (options_.comment_mode) {
    case CommentMode::kText:
      return FormatComment(candidate->text());
    case CommentMode::kComment:
      return candidate->comment();
    case CommentMode::kNone:
    default:
      return string();
  }
}

bool Rewriter::Transform(const an<Candidate>& candidate,
                         CandidateQueue* result) const {
  // Apply() already gates the translation on the current option state. Avoid
  // querying Context::get_option() again for every candidate in the hot path.
  if (!candidate || !result || !stage_) {
    return false;
  }
  if (options_.excluded_types.find(candidate->type()) !=
      options_.excluded_types.end()) {
    return false;
  }

  vector<string> values;
  if (!Rewrite(candidate->text(), &values)) {
    return false;
  }

  if (options_.mode == Mode::kComment) {
    const string annotation = FormatComment(Join(values, " "));
    string comment = annotation;
    if (options_.comment_mode == CommentMode::kComment &&
        !candidate->comment().empty()) {
      comment = candidate->comment() + " " + annotation;
    }
    result->push_back(New<ShadowCandidate>(candidate, candidate->type(),
                                           candidate->text(), comment, false));
    return true;
  }

  const string comment = DerivedComment(candidate);
  const bool append_mode = options_.mode == Mode::kAppend;
  if (append_mode) {
    result->push_back(candidate);
  }

  const string type = CandidateType(candidate);
  for (const auto& value : values) {
    // The compiler deduplicates values for each key. The only duplicate that
    // matters here is an append result equal to the original candidate.
    if (value.empty() || (append_mode && value == candidate->text())) {
      continue;
    }
    result->push_back(
        New<ShadowCandidate>(candidate, type, value, comment, false));
  }

  if (options_.mode == Mode::kReplace && result->empty()) {
    result->push_back(candidate);
  }
  return true;
}

an<Translation> Rewriter::Apply(an<Translation> translation,
                                CandidateList* candidates) {
  (void)candidates;
  if (!translation || !stage_ || !Active()) {
    return translation;
  }
  if (options_.mode == Mode::kAbbrev) {
    return New<RewriterAbbrevTranslation>(translation, this,
                                          engine_->context());
  }
  return New<RewriterTranslation>(translation, this);
}

}  // namespace rime
