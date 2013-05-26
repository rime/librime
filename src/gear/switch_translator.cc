//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-05-26 GONG Chen <chen.sst@gmail.com>
//

#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/translation.h>
#include <rime/gear/switch_translator.h>

static const char* kRightArrow = " \xe2\x86\x92 ";

namespace rime {

class Switch : public SimpleCandidate, public SwitcherCommand {
 public:
  Switch(const std::string &current_state_label,
         const std::string &next_state_label,
         const std::string &option_name,
         bool current_state,
         bool auto_save)
      : SimpleCandidate("switch", 0, 0,
                        current_state_label + kRightArrow + next_state_label),
        SwitcherCommand(option_name),
        target_state_(!current_state),
        auto_save_(auto_save) {
  }
  virtual void Apply(Switcher* switcher);

 protected:
  bool target_state_;
  bool auto_save_;
};

void Switch::Apply(Switcher* switcher) {
  Engine* target_engine = switcher->target_engine();
  if (!target_engine) return;
  target_engine->context()->set_option(keyword_, target_state_);
  if (auto_save_) {
    Config* user_config = switcher->user_config();
    if (user_config) {
      user_config->SetBool("var/option/" + keyword_, target_state_);
    }
  }
}

class SwitchTranslation : public FifoTranslation {
 public:
  SwitchTranslation(Switcher* switcher) {
    LoadSwitches(switcher);
  }
 protected:
  void LoadSwitches(Switcher* switcher);
};

void SwitchTranslation::LoadSwitches(Switcher* switcher) {
  Engine* target_engine = switcher->target_engine();
  if (!target_engine) return;
  Config *config = target_engine->schema()->config();
  if (!config) return;
  ConfigListPtr switches = config->GetList("switches");
  if (!switches) return;
  Context *context = target_engine->context();
  for (size_t i = 0; i < switches->size(); ++i) {
    ConfigMapPtr item = As<ConfigMap>(switches->GetAt(i));
    if (!item) continue;
    ConfigValuePtr option_name = item->GetValue("name");
    if (!option_name) continue;
    ConfigListPtr states = As<ConfigList>(item->Get("states"));
    if (!states || states->size() != 2) continue;
    bool current_state = context->get_option(option_name->str());
    Append(boost::make_shared<Switch>(
        states->GetValueAt(current_state)->str(),
        states->GetValueAt(1 - current_state)->str(),
        option_name->str(),
        current_state,
        switcher->IsAutoSave(option_name->str())));
  }
  DLOG(INFO) << "num switches: " << candies_.size();
}

SwitchTranslator::SwitchTranslator(const TranslatorTicket& ticket)
    : Translator(ticket) {
}

shared_ptr<Translation> SwitchTranslator::Query(const std::string& input,
                                                const Segment& segment,
                                                std::string* prompt) {
  Switcher* switcher = dynamic_cast<Switcher*>(engine_);
  if (!switcher) {
    return shared_ptr<Translation>();
  }
  return make_shared<SwitchTranslation>(switcher);
}

}  // namespace rime
