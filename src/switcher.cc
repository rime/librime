// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/processor.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/translation.h>

namespace rime {

class SwitcherOption : public Candidate {
 public:
  SwitcherOption(Schema *schema, const std::string &caption)
      : Candidate("schema", 0, caption.length()),
        schema_name_(schema->schema_name()),
        schema_id_(schema->schema_id()) {}
  
  const std::string& text() const { return schema_name_; }
  const std::string& schema_id() const { return schema_id_; }
  
 protected:
  std::string schema_name_;
  std::string schema_id_;
};

Switcher::Switcher() : Engine(new Schema),
                       target_engine_(NULL),
                       active_(false) {
  EZLOGGERFUNCTRACKER;
  // receive context notifications
  context_->select_notifier().connect(
      boost::bind(&Switcher::OnSelect, this, _1));

  user_config_.reset(Config::Require("config")->Create("user"));
  InitializeSubProcessors();
  LoadSettings();
}

Switcher::~Switcher() {
  EZLOGGERFUNCTRACKER;
}

void Switcher::Attach(Engine *engine) {
  target_engine_ = engine;
}

bool Switcher::ProcessKeyEvent(const KeyEvent &key_event) {
  EZLOGGERVAR(key_event);
  BOOST_FOREACH(const KeyEvent &hotkey, hotkeys_) {
    if (key_event == hotkey) {
      if (!active_ && target_engine_)
        Activate();
      else if (active_)
        Deactivate(); 
      return true;
    }
  }
  if (active_) {
    BOOST_FOREACH(shared_ptr<Processor> &p, processors_) {
      if (Processor::kNoop != p->ProcessKeyEvent(key_event))
        return true;
    }
    if (key_event.release() || key_event.ctrl() || key_event.alt())
      return true;
    int ch = key_event.keycode();
    if (ch == XK_space || ch == XK_Return) {
      context_->ConfirmCurrentSelection();
    }
    else if (ch == XK_Escape) {
      Deactivate();
    }
    return true;
  }
  return false;
}

Schema* Switcher::CreateSchema() {
  Config *config = schema_->config();
  if (!config) return NULL;
  ConfigListPtr schema_list = config->GetList("schema_list");
  if (!schema_list) return NULL;
  std::string previous;
  if (user_config_) {
    user_config_->GetString("var/previously_selected_schema", &previous);
  }
  std::string recent;
  for (size_t i = 0; i < schema_list->size(); ++i) {
    ConfigMapPtr item = As<ConfigMap>(schema_list->GetAt(i));
    if (!item) continue;
    ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    if (previous.empty() || previous == schema_id) {
      recent = schema_id;
      break;
    }
    if (recent.empty())
      recent = schema_id;
  }
  if (recent.empty())
    return NULL;
  else
    return new Schema(recent);
}

void Switcher::OnSelect(Context *ctx) {
  EZLOGGERFUNCTRACKER;
  Segment &seg(ctx->composition()->back());
  shared_ptr<SwitcherOption> option =
      As<SwitcherOption>(seg.GetSelectedCandidate());
  if (!option) return;
  if (target_engine_) {
    const std::string &schema_id(option->schema_id());
    const std::string &current_schema_id(target_engine_->schema()->schema_id());
    if (schema_id != current_schema_id) {
      target_engine_->set_schema(new Schema(schema_id));
    }
    // save history
    if (user_config_) {
      user_config_->SetString("var/previously_selected_schema", schema_id);
    }
  }
  Deactivate();
}

void Switcher::Activate() {
  EZLOGGERFUNCTRACKER;
  Config *config = schema_->config();
  if (!config) return;
  ConfigListPtr schema_list = config->GetList("schema_list");
  if (!schema_list) return;

  shared_ptr<FifoTranslation> switcher_options(new FifoTranslation);
  Schema *current_schema = NULL;
  // current schema comes first
  if (target_engine_ && target_engine_->schema()) {
    current_schema = target_engine_->schema();
    switcher_options->Append(
        shared_ptr<Candidate>(new SwitcherOption(current_schema, caption_)));
    // TODO: add custom switches
  }
  // load schema list
  for (size_t i = 0; i < schema_list->size(); ++i) {
    ConfigMapPtr item = As<ConfigMap>(schema_list->GetAt(i));
    if (!item) continue;
    ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    if (current_schema && schema_id == current_schema->schema_id())
      continue;
    scoped_ptr<Schema> schema(new Schema(schema_id));
    switcher_options->Append(
        shared_ptr<Candidate>(new SwitcherOption(schema.get(), caption_)));
  }
  // assign menu to switcher's context
  Composition *comp = context_->composition();
  if (!context_->IsComposing()) {
    // set caption
    context_->set_input(caption_);
    comp->Reset(caption_);
    Segment seg(0, caption_.length());
    comp->AddSegment(seg);
  }
  shared_ptr<Menu> menu(new Menu);
  comp->back().menu = menu;
  menu->AddTranslation(switcher_options);
  // activated!
  active_ = true;
}

void Switcher::Deactivate() {
  context_->Clear();
  active_ = false;
}

void Switcher::LoadSettings() {
  EZLOGGERFUNCTRACKER;
  Config *config = schema_->config();
  if (!config) return;
  if (!config->GetString("switcher/caption", &caption_) || caption_.empty()) {
    caption_ = ":-)";
  }
  ConfigListPtr hotkeys = config->GetList("switcher/hotkeys");
  if (!hotkeys) return;
  for (size_t i = 0; i < hotkeys->size(); ++i) {
    ConfigValuePtr value = hotkeys->GetValueAt(i);
    if (!value) continue;
    hotkeys_.push_back(KeyEvent(value->str()));
  }
}

void Switcher::InitializeSubProcessors() {
  EZLOGGERFUNCTRACKER;
  processors_.clear();
  {
    Processor::Component *c = Processor::Require("key_binder");
    if (!c) {
      EZLOGGERPRINT("Warning: key_binder not available.");
    }
    else {
      shared_ptr<Processor> p(c->Create(this));
      processors_.push_back(p);
    }
  }
  {
    Processor::Component *c = Processor::Require("selector");
    if (!c) {
      EZLOGGERPRINT("Warning: selector not available.");
    }
    else {
      shared_ptr<Processor> p(c->Create(this));
      processors_.push_back(p);
    }
  }
}

}  // namespace rime
