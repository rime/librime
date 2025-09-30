//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
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

class ConcreteEngine : public Engine {
 public:
  ConcreteEngine();
  virtual ~ConcreteEngine();
  virtual bool ProcessKey(const KeyEvent& key_event);
  virtual void ApplySchema(Schema* schema);
  virtual void ApplyOption(const string& name, bool value);
  virtual void CommitText(string text);
  virtual void Compose(Context* ctx);

 protected:
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

  vector<of<Processor>> processors_;
  vector<of<Segmentor>> segmentors_;
  vector<of<Translator>> translators_;
  vector<of<Filter>> filters_;
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

bool ConcreteEngine::ProcessKey(const KeyEvent& key_event) {
  DLOG(INFO) << "process key: " << key_event;
  ProcessResult ret = kNoop;
  for (auto& processor : processors_) {
    ret = processor->ProcessKeyEvent(key_event);
    if (ret == kRejected)
      break;
    if (ret == kAccepted)
      return true;
  }
  // record unhandled keys, eg. spaces, numbers, bksp's.
  context_->commit_history().Push(key_event);
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
    for (auto& segmentor : segmentors_) {
      if (!segmentor->Proceed(segments))
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
    for (auto& translator : translators_) {
      auto translation = translator->Query(input, segment);
      if (!translation)
        continue;
      if (translation->exhausted()) {
        DLOG(INFO) << translator->name_space() << " made a futile translation.";
        continue;
      }
      menu->AddTranslation(translation);
    }
    for (auto& filter : filters_) {
      if (filter->AppliesToSegment(&segment)) {
        menu->AddFilter(filter.get());
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
  schema_.reset(schema);
  context_->Clear();
  context_->ClearTransientOptions();
  InitializeComponents();
  InitializeOptions();
  switcher_->SetActiveSchema(schema_->schema_id());
  message_sink_("schema", schema_->schema_id() + "/" + schema_->schema_name());
}

void ConcreteEngine::ApplyOption(const string& name, bool value) {
  context_->set_option(name, value);
  if (switcher_->IsAutoSave(name)) {
    if (auto user_config = switcher_->user_config()) {
      user_config->SetBool("var/option/" + name, value);
    }
  }
}

// Helper template function to create components
template <typename T>
inline void CreateComponentsFromList(Engine* engine,
                                     Config* config,
                                     const string& config_key,
                                     const string& component_type,
                                     vector<an<T>>& target_collection) {
  if (auto component_list = config->GetList(config_key)) {
    size_t n = component_list->size();
    for (size_t i = 0; i < n; ++i) {
      auto prescription = As<ConfigValue>(component_list->GetAt(i));
      if (!prescription)
        continue;
      Ticket ticket{engine, component_type, prescription->str()};
      auto c = T::Require(ticket.klass);
      if (!c) {
        LOG(ERROR) << "error creating " << component_type << ": '"
                   << ticket.klass << "'";
        continue;
      }
      auto component = c->Create(ticket);
      if (!component) {
        LOG(ERROR) << "error creating " << component_type << " from ticket: '"
                   << ticket.klass << "'";
        continue;
      }
      an<T> instance(component);
      target_collection.push_back(instance);
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
    processors_.push_back(switcher_);
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
    formatters_.push_back(f);
  } else {
    LOG(WARNING) << "shape_formatter not available.";
  }
  // create post-processors
  auto c_processor = Processor::Require("shape_processor");
  if (c_processor) {
    an<Processor> p(c_processor->Create(Ticket(this)));
    post_processors_.push_back(p);
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
