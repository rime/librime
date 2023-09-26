//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/processor.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

Switcher::Switcher(const Ticket& ticket) : Processor(ticket) {
  context_->set_option("dumb", true);  // not going to commit anything

  // receive context notifications
  context_->select_notifier().connect([this](Context* ctx) { OnSelect(ctx); });

  user_config_.reset(Config::Require("user_config")->Create("user"));
  InitializeComponents();
  LoadSettings();
  RestoreSavedOptions();
}

Switcher::~Switcher() {
  if (active_) {
    Deactivate();
  }
}

void Switcher::RestoreSavedOptions() {
  if (user_config_) {
    for (const string& option_name : save_options_) {
      bool value = false;
      if (user_config_->GetBool("var/option/" + option_name, &value)) {
        engine_->context()->set_option(option_name, value);
      }
    }
  }
}

ProcessResult Switcher::ProcessKeyEvent(const KeyEvent& key_event) {
  for (const KeyEvent& hotkey : hotkeys_) {
    if (key_event == hotkey) {
      if (!active_ && engine_) {
        Activate();
      } else if (active_) {
        HighlightNextSchema();
      }
      return kAccepted;
    }
  }
  if (active_) {
    for (auto& p : processors_) {
      ProcessResult result = p->ProcessKeyEvent(key_event);
      if (result != kNoop) {
        return result;
      }
    }
    if (key_event.release() || key_event.ctrl() || key_event.alt()) {
      return kAccepted;
    }
    int ch = key_event.keycode();
    if (ch == XK_space || ch == XK_Return) {
      context_->ConfirmCurrentSelection();
    } else if (ch == XK_Escape) {
      Deactivate();
    }
    return kAccepted;
  }
  return kNoop;
}

void Switcher::HighlightNextSchema() {
  Composition& comp = context_->composition();
  if (comp.empty() || !comp.back().menu)
    return;
  Segment& seg(comp.back());
  int index = seg.selected_index;
  an<Candidate> option;
  do {
    ++index;  // next
    int candidate_count = seg.menu->Prepare(index + 1);
    if (candidate_count <= index) {
      index = 0;  // passed the end; rewind
      break;
    } else {
      option = seg.GetCandidateAt(index);
    }
  } while (!option || option->type() != "schema");
  seg.selected_index = index;
  seg.tags.insert("paging");
  return;
}

/*
  Example configuration:

  ```yaml
  schema_list:
  - case: [mode/wubi, mode/wubi_pinyin]
    schema: wubi_pinyin
  - case: [mode/wubi]
    schema: wubi86
  - case: [mode/default]
    schema: pinyin

  mode:
    wubi: false
    wubi_pinyin: false
    default: true
  ```
*/

static an<ConfigValue> ParseSchemaListEntry(Config* config,
                                            an<ConfigMap> entry_map) {
  if (!entry_map)
    return nullptr;
  auto schema_property = entry_map->GetValue("schema");
  if (!schema_property)
    return nullptr;
  if (auto case_conditions = As<ConfigList>(entry_map->Get("case"))) {
    for (auto iter = case_conditions->begin(); iter != case_conditions->end();
         ++iter) {
      if (auto condition_variable = As<ConfigValue>(*iter)) {
        bool condition_met = false;
        if (!config->GetBool(condition_variable->str(), &condition_met) ||
            !condition_met) {
          return nullptr;
        }
      }
    }
  }
  return schema_property;
}

int Switcher::ForEachSchemaListEntry(
    Config* config,
    function<bool(const string& schema_id)> process_entry) {
  auto schema_list = config->GetList("schema_list");
  if (!schema_list)
    return 0;
  int num_processed_entries = 0;
  for (auto iter = schema_list->begin(); iter != schema_list->end(); ++iter) {
    auto entry = ParseSchemaListEntry(config, As<ConfigMap>(*iter));
    if (!entry)
      continue;
    const string& schema_id = entry->str();
    ++num_processed_entries;
    if (!process_entry(schema_id))
      break;
  }
  return num_processed_entries;
}

Schema* Switcher::CreateSchema() {
  Config* config = schema_->config();
  if (!config)
    return nullptr;
  string previous;
  if (user_config_ && !fix_schema_list_order_) {
    user_config_->GetString("var/previously_selected_schema", &previous);
  }
  string recent;
  ForEachSchemaListEntry(config, [&previous, &recent](const string& schema_id) {
    if (previous.empty() || previous == schema_id) {
      recent = schema_id;
      return /* continue = */ false;
    }
    if (recent.empty())
      recent = schema_id;
    return /* continue = */ true;
  });
  if (recent.empty())
    return nullptr;
  else
    return new Schema(recent);
}

void Switcher::SelectNextSchema() {
  if (translators_.empty())
    return;
  auto xlator = translators_[0];  // schema_list_translator
  if (!xlator)
    return;
  Menu menu;
  menu.AddTranslation(xlator->Query("", Segment()));
  if (menu.Prepare(2) < 2)
    return;
  auto command = As<SwitcherCommand>(menu.GetCandidateAt(1));
  if (!command)
    return;
  command->Apply(this);
}

bool Switcher::IsAutoSave(const string& option) const {
  return save_options_.find(option) != save_options_.end();
}

void Switcher::OnSelect(Context* ctx) {
  LOG(INFO) << "a switcher option is selected.";
  if (auto command = As<SwitcherCommand>(ctx->GetSelectedCandidate())) {
    command->Apply(this);
  }
}

void Switcher::RefreshMenu() {
  Composition& comp = context_->composition();
  if (comp.empty()) {
    // no longer need this to make context_->IsComposing() == true
    // context_->set_input(" ");
    Segment seg(0, 0);  // empty range
    seg.prompt = caption_;
    comp.AddSegment(seg);
  }
  auto menu = New<Menu>();
  comp.back().menu = menu;
  for (auto& translator : translators_) {
    if (auto t = translator->Query("", comp.back())) {
      menu->AddTranslation(t);
    }
  }
}

void Switcher::Activate() {
  LOG(INFO) << "switcher is activated.";
  context_->set_option("_fold_options", fold_options_);
  RefreshMenu();
  engine_->set_active_engine(this);
  active_ = true;
}

void Switcher::Deactivate() {
  context_->Clear();
  engine_->set_active_engine();
  active_ = false;
}

void Switcher::LoadSettings() {
  Config* config = schema_->config();
  if (!config)
    return;
  if (!config->GetString("switcher/caption", &caption_) || caption_.empty()) {
    caption_ = ":-)";
  }
  if (auto hotkeys = config->GetList("switcher/hotkeys")) {
    hotkeys_.clear();
    for (size_t i = 0; i < hotkeys->size(); ++i) {
      auto value = hotkeys->GetValueAt(i);
      if (!value)
        continue;
      hotkeys_.push_back(KeyEvent(value->str()));
    }
  }
  if (auto options = config->GetList("switcher/save_options")) {
    save_options_.clear();
    for (auto it = options->begin(); it != options->end(); ++it) {
      auto option_name = As<ConfigValue>(*it);
      if (!option_name)
        continue;
      save_options_.insert(option_name->str());
    }
  }
  config->GetBool("switcher/fold_options", &fold_options_);
  config->GetBool("switcher/fix_schema_list_order", &fix_schema_list_order_);
}

void Switcher::InitializeComponents() {
  processors_.clear();
  translators_.clear();
  if (auto c = Processor::Require("key_binder")) {
    an<Processor> p(c->Create(Ticket(this)));
    processors_.push_back(p);
  } else {
    LOG(WARNING) << "key_binder not available.";
  }
  if (auto c = Processor::Require("selector")) {
    an<Processor> p(c->Create(Ticket(this)));
    processors_.push_back(p);
  } else {
    LOG(WARNING) << "selector not available.";
  }
  DLOG(INFO) << "num processors: " << processors_.size();
  if (auto c = Translator::Require("schema_list_translator")) {
    an<Translator> t(c->Create(Ticket(this)));
    translators_.push_back(t);
  } else {
    LOG(WARNING) << "schema_list_translator not available.";
  }
  if (auto c = Translator::Require("switch_translator")) {
    an<Translator> t(c->Create(Ticket(this)));
    translators_.push_back(t);
  } else {
    LOG(WARNING) << "switch_translator not available.";
  }
  DLOG(INFO) << "num translators: " << translators_.size();
}

}  // namespace rime
