//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <chrono>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/config/config_types.h>
#include <rime/engine.h>
#include <rime/filter.h>
#include <rime/formatter.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/processor.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/segmentor.h>
#include <rime/switcher.h>
#include <rime/switches.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

struct Condition {
  enum Type { kOption, kAllOf, kAnyOf, kNoneOf } type = kOption;
  string option_name;
  bool expected_value = true;
  vector<Condition> children;
};

template <typename T>
struct ComponentEntry {
  an<T> component;
  Condition condition;
  bool has_condition = false;
  bool enabled = true;
};

inline bool EvaluateCondition(const Condition& condition, Context* ctx) {
  if (!ctx)
    return false;
  switch (condition.type) {
    case Condition::kOption:
      return ctx->get_option(condition.option_name) == condition.expected_value;
    case Condition::kAllOf:
      for (const auto& child : condition.children) {
        if (!EvaluateCondition(child, ctx))
          return false;
      }
      return true;
    case Condition::kAnyOf:
      for (const auto& child : condition.children) {
        if (EvaluateCondition(child, ctx))
          return true;
      }
      return false;
    case Condition::kNoneOf:
      for (const auto& child : condition.children) {
        if (EvaluateCondition(child, ctx))
          return false;
      }
      return true;
  }
  return false;
}

template <typename T>
inline void UpdateComponentEnabled(ComponentEntry<T>& entry, Context* ctx) {
  if (!entry.has_condition) {
    entry.enabled = true;
    return;
  }
  entry.enabled = EvaluateCondition(entry.condition, ctx);
}

template <typename T>
inline void UpdateComponentConditions(vector<ComponentEntry<T>>& components,
                                      Context* ctx) {
  for (auto& entry : components) {
    UpdateComponentEnabled(entry, ctx);
  }
}

class ConcreteEngine : public Engine {
 public:
  ConcreteEngine();
  virtual ~ConcreteEngine();
  virtual bool ProcessKey(const KeyEvent& key_event) {
    return ProcessKey(key_event, /* synthetic= */ false);
  }
  virtual bool ProcessSyntheticKey(const KeyEvent& key_event) {
    return ProcessKey(key_event, /* synthetic= */ true);
  }
  virtual void ApplySchema(Schema* schema);
  virtual void CommitText(string text);
  virtual void Compose(Context* ctx);

 protected:
  bool ProcessKey(const KeyEvent& key_event, bool synthetic);
  void InitializeComponents();
  void InitializeOptions();
  void CalculateSegmentation(Segmentation* segments);
  void TranslateSegments(Segmentation* segments);
  void FormatText(string* text);
  void OnCommit(Context* ctx);
  void OnSelect(Context* ctx);
  void OnContextUpdate(Context* ctx);
  void OnOptionUpdate(Context* ctx, const string& option);
  void OnPropertyUpdate(Context* ctx, const string& property);

  vector<ComponentEntry<Processor>> processors_;
  vector<ComponentEntry<Segmentor>> segmentors_;
  vector<ComponentEntry<Translator>> translators_;
  vector<ComponentEntry<Filter>> filters_;
  vector<of<Formatter>> formatters_;
  vector<of<Processor>> post_processors_;
  an<Switcher> switcher_;
};

// implementations

Engine* Engine::Create() {
  return new ConcreteEngine;
}

Engine::Engine() : schema_(new Schema), context_(new Context) {}

Engine::~Engine() {
  context_.reset();
  schema_.reset();
}

ConcreteEngine::ConcreteEngine() {
  LOG(INFO) << "starting engine.";
  // receive context notifications
  context_->commit_notifier().connect([this](Context* ctx) { OnCommit(ctx); });
  context_->select_notifier().connect([this](Context* ctx) { OnSelect(ctx); });
  context_->update_notifier().connect(
      [this](Context* ctx) { OnContextUpdate(ctx); });
  context_->option_update_notifier().connect(
      [this](Context* ctx, const string& option) {
        OnOptionUpdate(ctx, option);
      });
  context_->property_update_notifier().connect(
      [this](Context* ctx, const string& property) {
        OnPropertyUpdate(ctx, property);
      });

  switcher_ = New<Switcher>(this);
  // saved options should be loaded only once per input session
  switcher_->RestoreSavedOptions();

  InitializeComponents();
  InitializeOptions();
}

ConcreteEngine::~ConcreteEngine() {
  LOG(INFO) << "engine disposed.";
}

bool ConcreteEngine::ProcessKey(const KeyEvent& key_event, bool synthetic) {
  DLOG(INFO) << "process key: " << key_event << ", synthetic: " << synthetic;
  ProcessResult ret = kNoop;
  for (auto& entry : processors_) {
    if (!entry.enabled)
      continue;
    ret = entry.component->ProcessKeyEvent(key_event);
    if (ret == kRejected)
      break;
    if (ret == kAccepted)
      return true;
  }
  // record unhandled keys, eg. spaces, numbers, bksp's.
  // skip internal synthetic key events which won't reach the client app.
  if (!synthetic) {
    context_->commit_history().Push(key_event);
  }
  // post-processing
  for (auto& processor : post_processors_) {
    ret = processor->ProcessKeyEvent(key_event);
    if (ret == kRejected)
      break;
    if (ret == kAccepted)
      return true;
  }
  // notify interested parties
  context_->unhandled_key_notifier()(context_.get(), key_event);
  return false;
}

void ConcreteEngine::OnContextUpdate(Context* ctx) {
  if (!ctx)
    return;
  Compose(ctx);
}

void ConcreteEngine::OnOptionUpdate(Context* ctx, const string& option) {
  if (!ctx)
    return;
  LOG(INFO) << "updated option: " << option;
  UpdateComponentConditions(processors_, ctx);
  UpdateComponentConditions(segmentors_, ctx);
  UpdateComponentConditions(translators_, ctx);
  UpdateComponentConditions(filters_, ctx);
  // apply new option to active segment
  if (ctx->IsComposing()) {
    ctx->RefreshNonConfirmedComposition();
  }
  // notification
  bool option_is_on = ctx->get_option(option);
  string msg(option_is_on ? option : "!" + option);
  message_sink_("option", msg);
}

void ConcreteEngine::OnPropertyUpdate(Context* ctx, const string& property) {
  if (!ctx)
    return;
  LOG(INFO) << "updated property: " << property;
  // notification
  string value = ctx->get_property(property);
  string msg(property + "=" + value);
  message_sink_("property", msg);
}

void ConcreteEngine::Compose(Context* ctx) {
  if (!ctx)
    return;
  Composition& comp = ctx->composition();
  const string active_input = ctx->input().substr(0, ctx->caret_pos());
  DLOG(INFO) << "active input: " << active_input;
  comp.Reset(active_input);
  if (ctx->caret_pos() < ctx->input().length() &&
      ctx->caret_pos() == comp.GetConfirmedPosition()) {
    // translate one segment past caret pos.
    comp.Reset(ctx->input());
  }
  CalculateSegmentation(&comp);
  TranslateSegments(&comp);
  DLOG(INFO) << "composition: [" << comp.GetDebugText() << "]";
}

void ConcreteEngine::CalculateSegmentation(Segmentation* segments) {
  DLOG(INFO) << "CalculateSegmentation, segments: " << segments->size()
             << ", finished? " << segments->HasFinishedSegmentation();
  while (!segments->HasFinishedSegmentation()) {
    size_t start_pos = segments->GetCurrentStartPosition();
    size_t end_pos = segments->GetCurrentEndPosition();
    DLOG(INFO) << "start pos: " << start_pos;
    DLOG(INFO) << "end pos: " << end_pos;
    // recognize a segment by calling the segmentors in turn
    for (auto& entry : segmentors_) {
      if (!entry.enabled)
        continue;
      if (!entry.component->Proceed(segments))
        break;
    }
    DLOG(INFO) << "segmentation: " << *segments;
    // no advancement
    if (start_pos == segments->GetCurrentEndPosition())
      break;
    // only one segment is allowed past caret pos, which is the segment
    // immediately after the caret.
    if (start_pos >= context_->caret_pos())
      break;
    // move onto the next segment...
    if (!segments->HasFinishedSegmentation())
      segments->Forward();
  }
  // start an empty segment only at the end of a confirmed composition.
  if (!segments->empty() && !segments->back().HasTag("placeholder"))
    segments->Trim();
  if (!segments->empty() && segments->back().status >= Segment::kSelected)
    segments->Forward();
}

void ConcreteEngine::TranslateSegments(Segmentation* segments) {
  DLOG(INFO) << "TranslateSegments: " << *segments;
  for (Segment& segment : *segments) {
    DLOG(INFO) << "segment [" << segment.start << ", " << segment.end
               << "), status: " << segment.status;
    if (segment.status >= Segment::kGuess)
      continue;
    size_t len = segment.end - segment.start;
    string input = segments->input().substr(segment.start, len);
    DLOG(INFO) << "translating segment: [" << input << "]";
    auto menu = New<Menu>();
    for (auto& entry : translators_) {
      if (!entry.enabled)
        continue;
      auto translation = entry.component->Query(input, segment);
      if (!translation)
        continue;
      if (translation->exhausted()) {
        DLOG(INFO) << entry.component->name_space()
                   << " made a futile translation.";
        continue;
      }
      menu->AddTranslation(translation);
    }
    for (auto& entry : filters_) {
      if (entry.enabled && entry.component->AppliesToSegment(&segment)) {
        menu->AddFilter(entry.component.get());
      }
    }
    segment.status = Segment::kGuess;
    segment.menu = menu;
    segment.selected_index = 0;
  }
}

void ConcreteEngine::FormatText(string* text) {
  if (formatters_.empty())
    return;
  DLOG(INFO) << "applying formatters.";
  for (auto& formatter : formatters_) {
    formatter->Format(text);
  }
}

void ConcreteEngine::CommitText(string text) {
  context_->commit_history().Push(CommitRecord{"raw", text});
  FormatText(&text);
  DLOG(INFO) << "committing text: " << text;
  sink_(text);
}

void ConcreteEngine::OnCommit(Context* ctx) {
  context_->commit_history().Push(ctx->composition(), ctx->input());
  string text = ctx->GetCommitText();
  FormatText(&text);
  DLOG(INFO) << "committing composition: " << text;
  sink_(text);
}

void ConcreteEngine::OnSelect(Context* ctx) {
  Segment& seg(ctx->composition().back());
  seg.Close();
  if (seg.end == ctx->input().length()) {
    // composition has finished
    seg.status = Segment::kConfirmed;
    // strategy one: commit directly;
    // strategy two: continue composing with another empty segment.
    if (ctx->get_option("_auto_commit"))
      ctx->Commit();
    else
      ctx->composition().Forward();
  } else {
    bool reached_caret_pos = (seg.end >= ctx->caret_pos());
    ctx->composition().Forward();
    if (reached_caret_pos) {
      // finished converting current segment
      // move caret to the end of input
      ctx->set_caret_pos(ctx->input().length());
    } else {
      Compose(ctx);
    }
  }
}

void ConcreteEngine::ApplySchema(Schema* schema) {
  if (!schema)
    return;
  // 支持 this->schema() 原位重新加載
  if (schema != schema_.get()) {
    schema_.reset(schema);
  }
  context_->Clear();
  context_->ClearTransientOptions();
  InitializeComponents();
  InitializeOptions();
  switcher_->SetActiveSchema(schema_->schema_id());
  message_sink_("schema", schema_->schema_id() + "/" + schema_->schema_name());
}

// Helper template function to create components
inline bool HasOnlyKeys(ConfigMap* map, const set<string>& allowed_keys) {
  if (!map)
    return false;
  for (auto it = map->begin(); it != map->end(); ++it) {
    if (allowed_keys.find(it->first) == allowed_keys.end())
      return false;
  }
  return true;
}

inline bool HasExactKeys(ConfigMap* map, const set<string>& expected_keys) {
  if (!map)
    return false;
  size_t key_count = 0;
  for (auto it = map->begin(); it != map->end(); ++it)
    ++key_count;
  if (key_count != expected_keys.size())
    return false;
  return HasOnlyKeys(map, expected_keys);
}

constexpr int kMaxConditionDepth = 32;

inline bool ParseCondition(const an<ConfigItem>& item,
                           Condition* condition,
                           int depth = 0) {
  if (!condition || depth > kMaxConditionDepth)
    return false;

  if (auto value = As<ConfigValue>(item)) {
    string option = value->str();
    if (option.empty())
      return false;
    condition->type = Condition::kOption;
    condition->expected_value = true;
    if (option[0] == '!') {
      condition->expected_value = false;
      option.erase(0, 1);
    }
    if (option.empty())
      return false;
    condition->option_name = option;
    condition->children.clear();
    return true;
  }

  auto map = As<ConfigMap>(item);
  if (!map)
    return false;

  // Explicit option conditions are useful when a boolean value is needed.
  if (map->HasKey("option")) {
    if (!HasExactKeys(map.get(), {"option", "value"}))
      return false;
    auto option = map->GetValue("option");
    auto value = map->GetValue("value");
    if (!option || option->str().empty() || !value ||
        !value->GetBool(&condition->expected_value))
      return false;
    condition->type = Condition::kOption;
    condition->option_name = option->str();
    condition->children.clear();
    return true;
  }

  string operation;
  Condition::Type type = Condition::kOption;
  if (map->HasKey("all_of")) {
    operation = "all_of";
    type = Condition::kAllOf;
  } else if (map->HasKey("any_of")) {
    operation = "any_of";
    type = Condition::kAnyOf;
  } else if (map->HasKey("none_of")) {
    operation = "none_of";
    type = Condition::kNoneOf;
  } else {
    return false;
  }
  if (!HasExactKeys(map.get(), {operation}))
    return false;

  auto list = As<ConfigList>(map->Get(operation));
  if (!list || list->empty())
    return false;
  condition->type = type;
  condition->option_name.clear();
  condition->children.clear();
  for (size_t i = 0; i < list->size(); ++i) {
    Condition child;
    if (!ParseCondition(list->GetAt(i), &child, depth + 1))
      return false;
    condition->children.push_back(child);
  }
  return true;
}

template <typename T>
inline bool ParseComponentSpec(const an<ConfigItem>& item,
                               string* prescription,
                               ComponentEntry<T>* entry) {
  if (auto value = As<ConfigValue>(item)) {
    *prescription = value->str();
    return true;
  }

  auto map = As<ConfigMap>(item);
  if (!map || !HasOnlyKeys(map.get(), {"name", "when"}))
    return false;

  auto name = map->GetValue("name");
  if (!name || name->str().empty())
    return false;
  *prescription = name->str();

  auto when_node = map->Get("when");
  if (!when_node)
    return true;
  if (!ParseCondition(when_node, &entry->condition))
    return false;
  entry->has_condition = true;
  return true;
}

template <typename T>
inline void CreateComponentsFromList(
    Engine* engine,
    Config* config,
    const string& config_key,
    const string& component_type,
    vector<ComponentEntry<T>>& target_collection) {
  if (auto component_list = config->GetList(config_key)) {
    size_t n = component_list->size();
    for (size_t i = 0; i < n; ++i) {
      string prescription;
      ComponentEntry<T> entry;
      if (!ParseComponentSpec<T>(component_list->GetAt(i), &prescription,
                                 &entry)) {
        LOG(ERROR) << "invalid " << component_type << " prescription at index "
                   << i << " in '" << config_key << "'";
        continue;
      }
      Ticket ticket{engine, component_type, prescription};
      auto c = T::Require(ticket.klass);
      if (!c) {
        LOG(ERROR) << "error creating " << component_type << ": '"
                   << ticket.klass << "'";
        continue;
      }
      const auto create_start = std::chrono::steady_clock::now();
      auto component = c->Create(ticket);
      const auto create_cost_us =
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - create_start)
              .count();
      const auto create_cost_ms = create_cost_us / 1000.0;
      LOG(INFO) << "[ComponentTiming] type=" << component_type
                << " prescription='" << prescription << "'"
                << " class='" << ticket.klass << "'"
                << " namespace='" << ticket.name_space << "'"
                << " cost_us=" << create_cost_us
                << " cost_ms=" << create_cost_ms
                << " success=" << (component != nullptr);
      if (!component) {
        LOG(ERROR) << "error creating " << component_type << " from ticket: '"
                   << ticket.klass << "'";
        continue;
      }
      an<T> instance(component);
      entry.component = instance;
      UpdateComponentEnabled(entry, engine->context());
      target_collection.push_back(entry);
    }
  }
}

void ConcreteEngine::InitializeComponents() {
  processors_.clear();
  segmentors_.clear();
  translators_.clear();
  filters_.clear();
  formatters_.clear();
  post_processors_.clear();

  if (switcher_) {
    ComponentEntry<Processor> entry;
    entry.component = switcher_;
    processors_.push_back(entry);
    if (schema_->schema_id() == ".default") {
      if (Schema* schema = switcher_->CreateSchema()) {
        schema_.reset(schema);
      }
    }
  }

  Config* config = schema_->config();
  if (!config)
    return;

  // Create components using inline template function
  CreateComponentsFromList<Processor>(this, config, "engine/processors",
                                      "processor", processors_);
  CreateComponentsFromList<Segmentor>(this, config, "engine/segmentors",
                                      "segmentor", segmentors_);
  CreateComponentsFromList<Translator>(this, config, "engine/translators",
                                       "translator", translators_);
  CreateComponentsFromList<Filter>(this, config, "engine/filters", "filter",
                                   filters_);
  // create formatters
  auto c_formatter = Formatter::Require("shape_formatter");
  if (c_formatter) {
    an<Formatter> f(c_formatter->Create(Ticket(this)));
    if (f) {
      formatters_.push_back(f);
    } else {
      LOG(WARNING) << "failed to create shape_formatter.";
    }
  } else {
    LOG(WARNING) << "shape_formatter not available.";
  }
  // create post-processors
  auto c_processor = Processor::Require("shape_processor");
  if (c_processor) {
    an<Processor> p(c_processor->Create(Ticket(this)));
    if (p) {
      post_processors_.push_back(p);
    } else {
      LOG(WARNING) << "failed to create shape_processor.";
    }
  } else {
    LOG(WARNING) << "shape_processor not available.";
  }
}

void ConcreteEngine::InitializeOptions() {
  LOG(INFO) << "ConcreteEngine::InitializeOptions";
  // reset custom switches
  Config* config = schema_->config();
  Switches switches(config);
  switches.FindOption([this](Switches::SwitchOption option) {
    LOG(INFO) << "found switch option: " << option.option_name
              << ", reset: " << option.reset_value;
    if (option.reset_value >= 0) {
      if (option.type == Switches::kToggleOption) {
        context_->set_option(option.option_name, (option.reset_value != 0));
      } else if (option.type == Switches::kRadioGroup) {
        context_->set_option(
            option.option_name,
            static_cast<int>(option.option_index) == option.reset_value);
      }
    }
    return Switches::kContinue;
  });
}

}  // namespace rime
