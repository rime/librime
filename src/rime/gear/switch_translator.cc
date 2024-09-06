//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-05-26 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/switches.h>
#include <rime/translation.h>
#include <rime/gear/switch_translator.h>

static const char* kRightArrow = "\xe2\x86\x92 ";
// static const char* kRadioSelected = " \xe2\x97\x89";  // U+25C9 FISHEYE
static const char* kRadioSelected = " \xe2\x9c\x93";  // U+2713 CHECK MARK

namespace rime {

using SwitchOption = Switches::SwitchOption;

inline static string get_state_label(const SwitchOption& option,
                                     size_t state_index,
                                     bool abbreviate = false) {
  return string(
      Switches::GetStateLabel(option.the_switch, state_index, abbreviate));
}

inline static bool has_state_label(const SwitchOption& option,
                                   size_t state_index) {
  return bool(Switches::GetStateLabel(option.the_switch, state_index, false));
}

class Switch : public SimpleCandidate, public SwitcherCommand {
 public:
  Switch(const SwitchOption& option, bool current_state, bool auto_save)
      : SimpleCandidate(
            "switch",
            0,
            0,
            get_state_label(option, current_state),
            kRightArrow + get_state_label(option, 1 - current_state)),
        SwitcherCommand(option.option_name),
        target_state_(!current_state),
        auto_save_(auto_save) {}
  void Apply(Switcher* switcher) override;

 protected:
  bool target_state_;
  bool auto_save_;
};

void Switch::Apply(Switcher* switcher) {
  switcher->DeactivateAndApply([this, switcher] {
    if (Engine* engine = switcher->attached_engine()) {
      engine->context()->set_option(keyword_, target_state_);
    }
    if (auto_save_) {
      if (Config* user_config = switcher->user_config()) {
        user_config->SetBool("var/option/" + keyword_, target_state_);
      }
    }
  });
}

class RadioOption;

class RadioGroup : public std::enable_shared_from_this<RadioGroup> {
 public:
  RadioGroup(Context* context, Switcher* switcher)
      : context_(context), switcher_(switcher) {}
  an<RadioOption> CreateOption(const SwitchOption& option, size_t option_index);
  void SelectOption(RadioOption* option);
  RadioOption* GetSelectedOption() const;

 private:
  Context* context_;
  Switcher* switcher_;
  vector<RadioOption*> options_;
};

class RadioOption : public SimpleCandidate, public SwitcherCommand {
 public:
  RadioOption(an<RadioGroup> group,
              const string& state_label,
              const string& option_name)
      : SimpleCandidate("switch", 0, 0, state_label),
        SwitcherCommand(option_name),
        group_(group) {}
  void Apply(Switcher* switcher) override;
  void UpdateState(bool selected);
  bool selected() const { return selected_; }

 protected:
  an<RadioGroup> group_;
  bool selected_ = false;
};

void RadioOption::Apply(Switcher* switcher) {
  switcher->DeactivateAndApply([this] { group_->SelectOption(this); });
}

void RadioOption::UpdateState(bool selected) {
  selected_ = selected;
  set_comment(selected ? kRadioSelected : "");
}

an<RadioOption> RadioGroup::CreateOption(const SwitchOption& option,
                                         size_t option_index) {
  auto radio_option = New<RadioOption>(shared_from_this(),
                                       get_state_label(option, option_index),
                                       option.option_name);
  options_.push_back(radio_option.get());
  return radio_option;
}

void RadioGroup::SelectOption(RadioOption* option) {
  if (!option)
    return;
  Config* user_config = switcher_->user_config();
  for (auto it = options_.begin(); it != options_.end(); ++it) {
    bool selected = (*it == option);
    (*it)->UpdateState(selected);
    const string& option_name((*it)->keyword());
    if (context_->get_option(option_name) != selected) {
      context_->set_option(option_name, selected);
      if (user_config && switcher_->IsAutoSave(option_name)) {
        user_config->SetBool("var/option/" + option_name, selected);
      }
    }
  }
}

RadioOption* RadioGroup::GetSelectedOption() const {
  if (options_.empty())
    return NULL;
  for (auto it = options_.begin(); it != options_.end(); ++it) {
    if (context_->get_option((*it)->keyword()))
      return *it;
  }
  return options_[0];
}

class FoldedOptions : public SimpleCandidate, public SwitcherCommand {
 public:
  FoldedOptions(Config* config)
      : SimpleCandidate("unfold", 0, 0, ""), SwitcherCommand("_fold_options") {
    LoadConfig(config);
  }
  void Apply(Switcher* switcher) override;
  void Append(const SwitchOption& option, size_t state_index);
  void Finish();

  size_t size() const { return labels_.size(); }

 private:
  void LoadConfig(Config* config);

  string prefix_;
  string suffix_;
  string separator_ = " ";
  bool abbreviate_options_ = false;

  vector<string> labels_;
};

void FoldedOptions::LoadConfig(Config* config) {
  if (!config) {
    return;
  }
  config->GetString("switcher/option_list_prefix", &prefix_);
  config->GetString("switcher/option_list_suffix", &suffix_);
  config->GetString("switcher/option_list_separator", &separator_);
  config->GetBool("switcher/abbreviate_options", &abbreviate_options_);
}

void FoldedOptions::Apply(Switcher* switcher) {
  // expand the list of options
  switcher->context()->set_option(keyword_, false);
  switcher->RefreshMenu();
}

void FoldedOptions::Append(const SwitchOption& option, size_t state_index) {
  labels_.push_back(get_state_label(option, state_index, abbreviate_options_));
}

void FoldedOptions::Finish() {
  text_ = prefix_ + boost::algorithm::join(labels_, separator_) + suffix_;
}

class SwitchTranslation : public FifoTranslation {
 public:
  SwitchTranslation(Switcher* switcher) { LoadSwitches(switcher); }

 protected:
  void LoadSwitches(Switcher* switcher);
};

void SwitchTranslation::LoadSwitches(Switcher* switcher) {
  Engine* engine = switcher->attached_engine();
  if (!engine)
    return;
  Config* config = engine->schema()->config();
  if (!config)
    return;
  Context* context = engine->context();
  vector<an<RadioGroup>> groups;
  Switches switches(config);
  switches.FindOption(
      [this, switcher, context,
       &groups](Switches::SwitchOption option) -> Switches::FindResult {
        if (!has_state_label(option, 0)) {
          return Switches::kContinue;
        }
        if (option.type == Switches::kToggleOption) {
          bool current_state = context->get_option(option.option_name);
          Append(New<Switch>(option, current_state,
                             switcher->IsAutoSave(option.option_name)));
        } else if (option.type == Switches::kRadioGroup) {
          an<RadioGroup> group;
          if (option.option_index == 0) {
            group = New<RadioGroup>(context, switcher);
            groups.push_back(group);
          } else {
            group = groups.back();
          }
          Append(group->CreateOption(option, option.option_index));
        }
        return Switches::kContinue;
      });
  for (auto& group : groups) {
    group->SelectOption(group->GetSelectedOption());
  }
  if (switcher->context()->get_option("_fold_options")) {
    auto folded_options = New<FoldedOptions>(switcher->schema()->config());
    switches.FindOption(
        [context, &folded_options](
            Switches::SwitchOption option) -> Switches::FindResult {
          bool current_state = context->get_option(option.option_name);
          if (option.type == Switches::kToggleOption) {
            if (has_state_label(option, current_state)) {
              folded_options->Append(option, current_state);
            }
          } else if (option.type == Switches::kRadioGroup) {
            if (current_state && has_state_label(option, option.option_index)) {
              folded_options->Append(option, option.option_index);
            }
          }
          return Switches::kContinue;
        });
    if (folded_options->size() > 1) {
      folded_options->Finish();
      candies_.clear();
      Append(folded_options);
    }
  }
  DLOG(INFO) << "num switches: " << candies_.size();
}

SwitchTranslator::SwitchTranslator(const Ticket& ticket) : Translator(ticket) {}

an<Translation> SwitchTranslator::Query(const string& input,
                                        const Segment& segment) {
  auto switcher = dynamic_cast<Switcher*>(engine_);
  if (!switcher) {
    return nullptr;
  }
  return New<SwitchTranslation>(switcher);
}

}  // namespace rime
