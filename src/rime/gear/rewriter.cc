//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/gear/rewriter.h>

#include <algorithm>
#include <mutex>
#include <set>
#include <utility>

#include <rime/candidate.h>
#include <rime/algo/strings.h>
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

void AppendUnique(string&& value, vector<string>* values) {
  if (std::find(values->begin(), values->end(), value) == values->end()) {
    values->push_back(std::move(value));
  }
}

void AppendUnique(RewriteResult&& value, vector<RewriteResult>* values) {
  const auto duplicate =
      std::find_if(values->begin(), values->end(), [&](const auto& item) {
        return item.value == value.value && item.preedit == value.preedit;
      });
  if (duplicate == values->end()) {
    values->push_back(std::move(value));
  }
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

std::mutex& RewriterRegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

std::set<Rewriter*>& RewriterRegistry() {
  static std::set<Rewriter*> registry;
  return registry;
}

path ResolveStorePath() {
  the<ResourceResolver> resolver(
      Service::instance().CreateDeployedResourceResolver(
          {"rewrite_pack", "", ".rwp"}));
  return resolver ? resolver->ResolvePath("rewriter") : path();
}

}  // namespace

class RewriterTranslation : public PrefetchTranslation {
 public:
  RewriterTranslation(an<Translation> translation,
                      const Rewriter* rewriter,
                      std::shared_ptr<const Rewriter::RuntimeState> state,
                      size_t segment_start,
                      bool has_segment_range)
      : PrefetchTranslation(translation),
        rewriter_(rewriter),
        state_(std::move(state)),
        segment_start_(segment_start),
        has_segment_range_(has_segment_range) {}

 protected:
  bool Replenish() override {
    auto candidate = translation_->Peek();
    translation_->Next();
    if (!candidate) {
      return false;
    }
    const size_t candidate_rank = candidate_rank_++;
    if (!rewriter_->Transform(*state_, candidate, &cache_, candidate_rank,
                              segment_start_, has_segment_range_)) {
      cache_.push_back(candidate);
    }
    return !cache_.empty();
  }

 private:
  const Rewriter* rewriter_;
  std::shared_ptr<const Rewriter::RuntimeState> state_;
  size_t segment_start_ = 0;
  bool has_segment_range_ = false;
  size_t candidate_rank_ = 0;
};

class RewriterAbbrevTranslation : public Translation {
 public:
  RewriterAbbrevTranslation(an<Translation> translation,
                            const Rewriter* rewriter,
                            std::shared_ptr<const Rewriter::RuntimeState> state,
                            Context* context,
                            size_t start,
                            size_t end)
      : translation_(translation),
        rewriter_(rewriter),
        state_(std::move(state)),
        context_(context),
        start_(start),
        end_(end) {
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
    if (state_->stages.empty() || !context_ || context_->input().empty()) {
      return;
    }

    if (end_ <= start_ || end_ > context_->input().size()) {
      return;
    }

    const string& context_input = context_->input();
    const std::string_view input(context_input.data() + start_, end_ - start_);
    vector<RewriteResult> values;
    if (!rewriter_->RewriteWithPreedit(*state_, input, &values)) {
      return;
    }
    const string type = rewriter_->options_.candidate_type.empty()
                            ? "abbrev"
                            : rewriter_->options_.candidate_type;
    for (const auto& value : values) {
      if (value.value.empty()) {
        continue;
      }
      auto candidate = New<SimpleCandidate>(type, start_, end_, value.value,
                                            string(), value.preedit);
      injected_.push_back(candidate);
    }
    front_limit_ = std::min(rewriter_->options_.insert_count, injected_.size());
  }

  void PrepareNext() {
    while (!current_) {
      // Peek once at the first visible candidate. promote_on_types is
      // explicitly configured because a filter cannot otherwise know
      // translator priority.
      if (!first_candidate_checked_) {
        first_candidate_checked_ = true;
        if (translation_) {
          first_candidate_ = translation_->Peek();
          if (first_candidate_ &&
              rewriter_->options_.promote_on_types.find(
                  first_candidate_->type()) !=
                  rewriter_->options_.promote_on_types.end()) {
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
        const bool has_regular_candidate =
            first_candidate_ || regular_yielded_ > 0 ||
            (translation_ && !translation_->exhausted());
        if (has_regular_candidate) {
          const size_t before = rewriter_->options_.insert_position > 0
                                    ? rewriter_->options_.insert_position - 1
                                    : 0;
          if (regular_yielded_ >= before) {
            insertion_started_ = true;
            current_ = injected_[injected_index_];
            current_is_injected_ = true;
            break;
          }
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
        if (regular_yielded_ == 0) {
          if (!rewriter_->options_.fallback_on_empty) {
            set_exhausted(true);
            break;
          }
          release_all_ = true;
          front_limit_ = injected_.size();
        }
        insertion_started_ = true;
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
  std::shared_ptr<const Rewriter::RuntimeState> state_;
  Context* context_;
  size_t start_;
  size_t end_;
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

Rewriter::Rewriter(const Ticket& ticket) : Filter(ticket), TagMatching(ticket) {
  if (name_space_.empty() || name_space_ == "filter") {
    name_space_ = "rewriter";
  }
  LoadConfig();

  if (engine_ && engine_->schema()) {
    schema_id_ = engine_->schema()->schema_id();
    store_path_ = ResolveStorePath();
  }

  if (options_.enable_sentence && engine_ && engine_->context()) {
    Context* context = engine_->context();
    commit_connection_ = context->commit_notifier().connect(
        [this](Context*) { ClearSentenceCache(); });
    abort_connection_ = context->abort_notifier().connect(
        [this](Context*) { ClearSentenceCache(); });
    update_connection_ =
        context->update_notifier().connect([this](Context* ctx) {
          if (!ctx || ctx->input().empty() || !ctx->IsComposing()) {
            ClearSentenceCache();
          }
        });
  }

  std::lock_guard<std::mutex> lock(RewriterRegistryMutex());
  RewriterRegistry().insert(this);
}

Rewriter::~Rewriter() {
  commit_connection_.disconnect();
  abort_connection_.disconnect();
  update_connection_.disconnect();
  ClearSentenceCache();

  std::lock_guard<std::mutex> lock(RewriterRegistryMutex());
  RewriterRegistry().erase(this);
}

void Rewriter::ReleaseStagesForDeployment() {
  std::lock_guard<std::mutex> registry_lock(RewriterRegistryMutex());
  for (Rewriter* rewriter : RewriterRegistry()) {
    if (!rewriter) {
      continue;
    }
    std::lock_guard<std::mutex> state_lock(rewriter->runtime_state_mutex_);
    std::atomic_store_explicit(&rewriter->runtime_state_,
                               std::shared_ptr<const RuntimeState>(),
                               std::memory_order_release);
    rewriter->runtime_state_load_attempted_ = false;
  }
}

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
    options_.mode = Mode::kDerive;
  }

  config->GetBool(name_space_ + "/enable_sentence", &options_.enable_sentence);
  config->GetBool(name_space_ + "/fallback_on_empty",
                  &options_.fallback_on_empty);

  string comment_source;
  config->GetString(name_space_ + "/comment_source", &comment_source);
  if (comment_source == "text") {
    options_.comment_source = CommentSource::kText;
  } else if (comment_source == "inherit") {
    options_.comment_source = CommentSource::kInherit;
  }
  config->GetString(name_space_ + "/comment_template",
                    &options_.comment_template);
  config->GetString(name_space_ + "/candidate_type", &options_.candidate_type);
  if (options_.candidate_type == "preserve") {
    options_.candidate_type.clear();
  }

  vector<string> excluded;
  ReadStringList(config, name_space_ + "/excluded_types", &excluded);
  options_.excluded_types.insert(excluded.begin(), excluded.end());

  vector<string> promoted;
  ReadStringList(config, name_space_ + "/promote_on_types", &promoted);
  options_.promote_on_types.insert(promoted.begin(), promoted.end());

  int value = 0;
  if (config->GetInt(name_space_ + "/insert_position", &value) && value > 0) {
    options_.insert_position = static_cast<size_t>(value);
  }
  if (config->GetInt(name_space_ + "/insert_count", &value) && value > 0) {
    options_.insert_count = static_cast<size_t>(value);
  }
}

bool Rewriter::AppliesToSegment(Segment* segment) {
  has_segment_range_ = false;
  if (!segment || !TagsMatch(segment)) {
    return false;
  }
  segment_start_ = segment->start;
  segment_end_ = segment->end;
  has_segment_range_ = true;
  return true;
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

std::shared_ptr<const Rewriter::RuntimeState> Rewriter::EnsureState() {
  auto state =
      std::atomic_load_explicit(&runtime_state_, std::memory_order_acquire);
  if (state) {
    return state;
  }

  std::lock_guard<std::mutex> lock(runtime_state_mutex_);
  state = std::atomic_load_explicit(&runtime_state_, std::memory_order_acquire);
  if (state) {
    return state;
  }
  if (runtime_state_load_attempted_ || schema_id_.empty() ||
      store_path_.empty()) {
    return nullptr;
  }
  runtime_state_load_attempted_ = true;

  auto store = RewriteStore::OpenCached(store_path_);
  if (!store) {
    LOG(WARNING) << "rewriter@" << name_space_
                 << " is disabled: shared rewrite store '" << store_path_
                 << "' could not be opened; redeploy the workspace.";
    return nullptr;
  }

  vector<RewriteStageId> stage_ids;
  if (!store->FindStageSequence(schema_id_, name_space_, &stage_ids)) {
    LOG(WARNING) << "rewriter@" << name_space_ << " is disabled: schema '"
                 << schema_id_ << "' has no deployed rewrite binding.";
    return nullptr;
  }

  auto loaded = std::make_shared<RuntimeState>();
  loaded->store = std::move(store);
  loaded->stages.reserve(stage_ids.size());
  for (const auto& stage_id : stage_ids) {
    auto pack = loaded->store->OpenStage(stage_id);
    if (!pack) {
      LOG(WARNING) << "rewriter@" << name_space_
                   << " is disabled: a deployed rewrite stage could not be "
                      "opened.";
      return nullptr;
    }
    loaded->stages.push_back({std::move(pack)});
  }
  if (loaded->stages.empty()) {
    return nullptr;
  }

  state = loaded;
  std::atomic_store_explicit(&runtime_state_, state, std::memory_order_release);
  return state;
}

void Rewriter::ClearSentenceCache() {
  for (auto& lane : sentence_lanes_) {
    lane.Reset();
  }
}

Rewriter::SentenceLane* Rewriter::PrepareSentenceLane(
    const RuntimeState& state,
    size_t candidate_rank,
    size_t segment_start,
    bool has_segment_range) const {
  if (!options_.enable_sentence || candidate_rank >= kSentenceCacheLanes ||
      !state.store) {
    return nullptr;
  }

  auto& lane = sentence_lanes_[candidate_rank];
  const uint64_t generation = state.store->generation();
  if (!lane.valid || lane.store_generation != generation ||
      lane.has_segment_range != has_segment_range ||
      (has_segment_range && lane.segment_start != segment_start) ||
      lane.stages.size() != state.stages.size()) {
    lane.Reset();
    lane.valid = true;
    lane.has_segment_range = has_segment_range;
    lane.segment_start = segment_start;
    lane.store_generation = generation;
    lane.stages.resize(state.stages.size());
  }
  return &lane;
}

bool Rewriter::Rewrite(const RuntimeState& state,
                       std::string_view text,
                       vector<string>* values,
                       size_t candidate_rank,
                       size_t segment_start,
                       bool has_segment_range) const {
  if (!values) {
    return false;
  }
  values->clear();
  if (state.stages.empty()) {
    return false;
  }

  if (options_.enable_sentence) {
    SentenceLane* lane = PrepareSentenceLane(state, candidate_rank,
                                             segment_start, has_segment_range);
    std::string_view current = text;
    bool matched = false;

    if (lane) {
      for (size_t i = 0; i < state.stages.size(); ++i) {
        auto& sentence_state = lane->stages[i];
        if (state.stages[i].pack->ConvertSentenceIncremental(current,
                                                             &sentence_state)) {
          current = sentence_state.output;
          matched = true;
        }
        // A miss is a no-op for this stage. The current text must still reach
        // later stages in a preset such as s2t -> t2hk.
      }
    } else {
      string current_storage(text);
      for (const auto& loaded : state.stages) {
        string rewritten;
        if (loaded.pack->ConvertSentence(current_storage, &rewritten)) {
          current_storage = std::move(rewritten);
          matched = true;
        }
      }
      if (!matched) {
        return false;
      }
      values->push_back(std::move(current_storage));
      return true;
    }

    if (!matched) {
      if (lane && lane->RetainedBytes() > kMaxSentenceCacheBytes) {
        lane->Reset();
      }
      return false;
    }

    values->emplace_back(current);
    if (lane && lane->RetainedBytes() > kMaxSentenceCacheBytes) {
      lane->Reset();
    }
    return true;
  }

  if (state.stages.size() == 1) {
    return state.stages.front().pack->Lookup(text, values);
  }

  vector<string> current;
  bool matched = state.stages.front().pack->Lookup(text, &current);
  if (!matched) {
    current.emplace_back(text);
  }

  for (size_t i = 1; i < state.stages.size(); ++i) {
    vector<string> next;
    next.reserve(current.size());
    vector<string> rewritten;
    for (auto& value : current) {
      if (state.stages[i].pack->Lookup(value, &rewritten)) {
        matched = true;
        for (auto& mapped : rewritten) {
          AppendUnique(std::move(mapped), &next);
        }
      } else {
        AppendUnique(std::move(value), &next);
      }
    }
    current.swap(next);
  }

  if (!matched) {
    return false;
  }
  *values = std::move(current);
  return true;
}

bool Rewriter::RewriteWithPreedit(const RuntimeState& state,
                                  std::string_view text,
                                  vector<RewriteResult>* values) const {
  if (!values) {
    return false;
  }
  values->clear();
  if (state.stages.empty()) {
    return false;
  }

  if (state.stages.size() == 1) {
    return state.stages.front().pack->LookupWithPreedit(text, values);
  }

  vector<RewriteResult> current;
  bool matched = state.stages.front().pack->LookupWithPreedit(text, &current);
  if (!matched) {
    current.push_back({string(text), string()});
  }

  for (size_t i = 1; i < state.stages.size(); ++i) {
    vector<RewriteResult> next;
    next.reserve(current.size());
    vector<RewriteResult> rewritten;
    for (auto& value : current) {
      if (state.stages[i].pack->LookupWithPreedit(value.value, &rewritten)) {
        matched = true;
        for (auto& mapped : rewritten) {
          if (!value.preedit.empty()) {
            mapped.preedit = value.preedit;
          }
          AppendUnique(std::move(mapped), &next);
        }
      } else {
        AppendUnique(std::move(value), &next);
      }
    }
    current.swap(next);
  }

  if (!matched) {
    return false;
  }
  *values = std::move(current);
  return true;
}

const string& Rewriter::CandidateType(const an<Candidate>& candidate) const {
  return options_.candidate_type.empty() ? candidate->type()
                                         : options_.candidate_type;
}

string Rewriter::ApplyCommentTemplate(const string& value) const {
  if (options_.comment_template.empty()) {
    return value;
  }
  string result = options_.comment_template;
  const size_t pos = result.find("%s");
  if (pos == string::npos) {
    return value;
  }
  result.replace(pos, 2, value);
  return result;
}

string Rewriter::DerivedComment(const an<Candidate>& candidate) const {
  string comment;
  switch (options_.comment_source) {
    case CommentSource::kText:
      comment = candidate->text();
      break;
    case CommentSource::kInherit:
      comment = candidate->comment();
      break;
    case CommentSource::kNone:
    default:
      return string();
  }
  return comment.empty() ? string() : ApplyCommentTemplate(comment);
}

bool Rewriter::Transform(const RuntimeState& state,
                         const an<Candidate>& candidate,
                         CandidateQueue* result,
                         size_t candidate_rank,
                         size_t segment_start,
                         bool has_segment_range) const {
  // Apply() already gates the translation on the current option state. Avoid
  // querying Context::get_option() again for every candidate in the hot path.
  if (!candidate || !result || state.stages.empty()) {
    return false;
  }
  if (options_.excluded_types.find(candidate->type()) !=
      options_.excluded_types.end()) {
    return false;
  }

  vector<string> values;
  if (!Rewrite(state, candidate->text(), &values, candidate_rank, segment_start,
               has_segment_range)) {
    return false;
  }

  if (options_.mode == Mode::kComment) {
    const string comment = ApplyCommentTemplate(strings::join(values, " "));
    result->push_back(New<ShadowCandidate>(candidate, candidate->type(),
                                           candidate->text(), comment, false));
    return true;
  }

  const string comment = DerivedComment(candidate);
  const bool derive_mode = options_.mode == Mode::kDerive;
  if (derive_mode) {
    result->push_back(candidate);
  }

  const string& type = CandidateType(candidate);
  for (const auto& value : values) {
    // The compiler deduplicates values for each key. The only duplicate that
    // matters here is a derive result equal to the original candidate.
    if (value.empty() || (derive_mode && value == candidate->text())) {
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
  const bool has_segment_range = has_segment_range_;
  const size_t segment_start = segment_start_;
  const size_t segment_end = segment_end_;
  has_segment_range_ = false;

  if (!translation) {
    return translation;
  }
  if (!Active()) {
    if (options_.enable_sentence) {
      ClearSentenceCache();
    }
    return translation;
  }
  auto state = EnsureState();
  if (!state) {
    if (options_.enable_sentence) {
      ClearSentenceCache();
    }
    return translation;
  }
  if (options_.mode == Mode::kAbbrev) {
    Context* context = engine_ ? engine_->context() : nullptr;
    if (!context || !has_segment_range) {
      return translation;
    }
    return New<RewriterAbbrevTranslation>(translation, this, std::move(state),
                                          context, segment_start, segment_end);
  }
  return New<RewriterTranslation>(translation, this, std::move(state),
                                  segment_start, has_segment_range);
}

}  // namespace rime
