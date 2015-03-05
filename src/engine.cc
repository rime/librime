//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <string>
#include <vector>
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
  virtual void CommitText(std::string text);
  virtual void Compose(Context* ctx);

 protected:
  void InitializeComponents();
  void InitializeOptions();
  void CalculateSegmentation(Segmentation* segments);
  void TranslateSegments(Segmentation* segments);
  void FormatText(std::string* text);
  void OnCommit(Context* ctx);
  void OnSelect(Context* ctx);
  void OnContextUpdate(Context* ctx);
  void OnOptionUpdate(Context* ctx, const std::string& option);

  std::vector<shared_ptr<Processor>> processors_;
  std::vector<shared_ptr<Segmentor>> segmentors_;
  std::vector<shared_ptr<Translator>> translators_;
  std::vector<shared_ptr<Filter>> filters_;
  std::vector<shared_ptr<Formatter>> formatters_;
  std::vector<shared_ptr<Processor>> post_processors_;
};

// implementations

Engine* Engine::Create() {
  return new ConcreteEngine;
}

Engine::Engine() : schema_(new Schema), context_(new Context) {
}

Engine::~Engine() {
  context_.reset();
  schema_.reset();
}

ConcreteEngine::ConcreteEngine() {
  LOG(INFO) << "starting engine.";
  // receive context notifications
  context_->commit_notifier().connect(
      [this](Context* ctx) { OnCommit(ctx); });
  context_->select_notifier().connect(
      [this](Context* ctx) { OnSelect(ctx); });
  context_->update_notifier().connect(
      [this](Context* ctx) { OnContextUpdate(ctx); });
  context_->option_update_notifier().connect(
      [this](Context* ctx, const std::string& option) {
        OnOptionUpdate(ctx, option);
      });
  InitializeComponents();
  InitializeOptions();
}

ConcreteEngine::~ConcreteEngine() {
  LOG(INFO) << "engine disposed.";
  processors_.clear();
  segmentors_.clear();
  translators_.clear();
}

bool ConcreteEngine::ProcessKey(const KeyEvent& key_event) {
  DLOG(INFO) << "process key: " << key_event;
  ProcessResult ret = kNoop;
  for (auto& processor : processors_) {
    ret = processor->ProcessKeyEvent(key_event);
    if (ret == kRejected) break;
    if (ret == kAccepted) return true;
  }
  // record unhandled keys, eg. spaces, numbers, bksp's.
  context_->commit_history().Push(key_event);
  // post-processing
  for (auto& processor : post_processors_) {
    ret = processor->ProcessKeyEvent(key_event);
    if (ret == kRejected) break;
    if (ret == kAccepted) return true;
  }
  // notify interested parties
  context_->unhandled_key_notifier()(context_.get(), key_event);
  return false;
}

void ConcreteEngine::OnContextUpdate(Context* ctx) {
  if (!ctx) return;
  Compose(ctx);
}

void ConcreteEngine::OnOptionUpdate(Context* ctx, const std::string& option) {
  if (!ctx) return;
  LOG(INFO) << "updated option: " << option;
  // apply new option to active segment
  if (ctx->IsComposing()) {
    ctx->RefreshNonConfirmedComposition();
  }
  // notification
  bool option_is_on = ctx->get_option(option);
  std::string msg(option_is_on ? option : "!" + option);
  message_sink_("option", msg);
}

void ConcreteEngine::Compose(Context* ctx) {
  if (!ctx) return;
  Composition& comp = ctx->composition();
  std::string active_input(ctx->input().substr(0, ctx->caret_pos()));
  DLOG(INFO) << "active input: " << active_input;
  comp.Reset(active_input);
  CalculateSegmentation(&comp);
  TranslateSegments(&comp);
  DLOG(INFO) << "composition: " << comp.GetDebugText();
}

void ConcreteEngine::CalculateSegmentation(Segmentation* segments) {
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
    // move onto the next segment...
    if (!segments->HasFinishedSegmentation())
      segments->Forward();
  }
  // start an empty segment only at the end of a confirmed composition.
  segments->Trim();
  if (!segments->empty() && segments->back().status >= Segment::kSelected)
    segments->Forward();
}

void ConcreteEngine::TranslateSegments(Segmentation* segments) {
  for (Segment& segment : *segments) {
    if (segment.status >= Segment::kGuess)
      continue;
    size_t len = segment.end - segment.start;
    if (len == 0)
      continue;
    std::string input = segments->input().substr(segment.start, len);
    DLOG(INFO) << "translating segment: " << input;
    auto menu = New<Menu>();
    for (auto& translator : translators_) {
      auto translation = translator->Query(input, segment);
      if (!translation)
        continue;
      if (translation->exhausted()) {
        LOG(INFO) << "Oops, got a futile translation.";
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

void ConcreteEngine::FormatText(std::string* text) {
  if (formatters_.empty())
    return;
  DLOG(INFO) << "applying formatters.";
  for (auto& formatter : formatters_) {
    formatter->Format(text);
  }
}

void ConcreteEngine::CommitText(std::string text) {
  context_->commit_history().Push(CommitRecord{"raw", text});
  FormatText(&text);
  DLOG(INFO) << "committing text: " << text;
  sink_(text);
}

void ConcreteEngine::OnCommit(Context* ctx) {
  context_->commit_history().Push(ctx->composition(), ctx->input());
  std::string text = ctx->GetCommitText();
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
  }
  else {
    ctx->composition().Forward();
    if (seg.end >= ctx->caret_pos()) {
      // finished converting current segment
      // move caret to the end of input
      ctx->set_caret_pos(ctx->input().length());
    }
    else {
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
  message_sink_("schema", schema->schema_id() + "/" + schema->schema_name());
}

void ConcreteEngine::InitializeComponents() {
  processors_.clear();
  segmentors_.clear();
  translators_.clear();
  filters_.clear();

  if (auto switcher = New<Switcher>(this)) {
    processors_.push_back(switcher);
    if (schema_->schema_id() == ".default") {
      if (Schema* schema = switcher->CreateSchema()) {
        schema_.reset(schema);
      }
    }
  }

  Config* config = schema_->config();
  if (!config)
    return;
  // create processors
  if (auto processor_list = config->GetList("engine/processors")) {
    size_t n = processor_list->size();
    for (size_t i = 0; i < n; ++i) {
      auto prescription = As<ConfigValue>(processor_list->GetAt(i));
      if (!prescription)
        continue;
      Ticket ticket{this, "processor", prescription->str()};
      if (auto c = Processor::Require(ticket.klass)) {
        shared_ptr<Processor> p(c->Create(ticket));
        processors_.push_back(p);
      }
      else {
        LOG(ERROR) << "error creating processor: '" << ticket.klass << "'";
      }
    }
  }
  // create segmentors
  if (auto segmentor_list = config->GetList("engine/segmentors")) {
    size_t n = segmentor_list->size();
    for (size_t i = 0; i < n; ++i) {
      auto prescription = As<ConfigValue>(segmentor_list->GetAt(i));
      if (!prescription)
        continue;
      Ticket ticket{this, "segmentor", prescription->str()};
      if (auto c = Segmentor::Require(ticket.klass)) {
        shared_ptr<Segmentor> s(c->Create(ticket));
        segmentors_.push_back(s);
      }
      else {
        LOG(ERROR) << "error creating segmentor: '" << ticket.klass << "'";
      }
    }
  }
  // create translators
  if (auto translator_list = config->GetList("engine/translators")) {
    size_t n = translator_list->size();
    for (size_t i = 0; i < n; ++i) {
      auto prescription = As<ConfigValue>(translator_list->GetAt(i));
      if (!prescription)
        continue;
      Ticket ticket{this, "translator", prescription->str()};
      if (auto c = Translator::Require(ticket.klass)) {
        shared_ptr<Translator> t(c->Create(ticket));
        translators_.push_back(t);
      }
      else {
        LOG(ERROR) << "error creating translator: '" << ticket.klass << "'";
      }
    }
  }
  // create filters
  if (auto filter_list = config->GetList("engine/filters")) {
    size_t n = filter_list->size();
    for (size_t i = 0; i < n; ++i) {
      auto prescription = As<ConfigValue>(filter_list->GetAt(i));
      if (!prescription)
        continue;
      Ticket ticket{this, "filter", prescription->str()};
      if (auto c = Filter::Require(ticket.klass)) {
        shared_ptr<Filter> f(c->Create(ticket));
        filters_.push_back(f);
      }
      else {
        LOG(ERROR) << "error creating filter: '" << ticket.klass << "'";
      }
    }
  }
  // create formatters
  if (auto c = Formatter::Require("shape_formatter")) {
    shared_ptr<Formatter> f(c->Create(Ticket(this)));
    formatters_.push_back(f);
  }
  else {
    LOG(WARNING) << "shape_formatter not available.";
  }
  // create post-processors
  if (auto c = Processor::Require("shape_processor")) {
    shared_ptr<Processor> p(c->Create(Ticket(this)));
    post_processors_.push_back(p);
  }
  else {
    LOG(WARNING) << "shape_processor not available.";
  }
}

void ConcreteEngine::InitializeOptions() {
  // reset custom switches
  Config* config = schema_->config();
  if (auto switches = config->GetList("switches")) {
    for (size_t i = 0; i < switches->size(); ++i) {
      auto item = As<ConfigMap>(switches->GetAt(i));
      if (!item)
        continue;
      auto reset_value = item->GetValue("reset");
      if (!reset_value)
        continue;
      int value = 0;
      reset_value->GetInt(&value);
      if (auto option_name = item->GetValue("name")) {
        // toggle
        context_->set_option(option_name->str(), (value != 0));
      }
      else if (auto options = As<ConfigList>(item->Get("options"))) {
        // radio
        for (size_t i = 0; i < options->size(); ++i) {
          if (auto option_name = options->GetValueAt(i)) {
            context_->set_option(option_name->str(),
                                 static_cast<int>(i) == value);
          }
        }
      }
    }
  }
}

}  // namespace rime
