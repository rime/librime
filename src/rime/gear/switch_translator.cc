//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-05-26 GONG Chen <chen.sst@gmail.com>
//
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/translation.h>
#include <rime/gear/switch_translator.h>

static const char* kRightArrow = "\xe2\x86\x92 ";
//static const char* kRadioSelected = " \xe2\x97\x89";  // U+25C9 FISHEYE
static const char* kRadioSelected = " \xe2\x9c\x93";  // U+2713 CHECK MARK

namespace rime {

class Switch : public SimpleCandidate, public SwitcherCommand {
 public:
  Switch(const string& current_state_label,
         const string& next_state_label,
         const string& option_name,
         bool current_state,
         bool auto_save)
      : SimpleCandidate("switch", 0, 0,
                        current_state_label, kRightArrow + next_state_label),
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
  if (Engine* engine = switcher->attached_engine()) {
    engine->context()->set_option(keyword_, target_state_);
  }
  if (auto_save_) {
    if (Config* user_config = switcher->user_config()) {
      user_config->SetBool("var/option/" + keyword_, target_state_);
    }
  }
  switcher->Deactivate();
}

class RadioOption;

class RadioGroup : public std::enable_shared_from_this<RadioGroup> {
 public:
  RadioGroup(Context* context, Switcher* switcher)
      : context_(context), switcher_(switcher) {
  }
  an<RadioOption> CreateOption(const string& state_label,
                                       const string& option_name);
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
        group_(group) {
  }
  virtual void Apply(Switcher* switcher);
  void UpdateState(bool selected);
  bool selected() const { return selected_; }

 protected:
  an<RadioGroup> group_;
  bool selected_ = false;
};

void RadioOption::Apply(Switcher* switcher) {
  group_->SelectOption(this);
  switcher->Deactivate();
}

void RadioOption::UpdateState(bool selected) {
  selected_ = selected;
  set_comment(selected ? kRadioSelected : "");
}

an<RadioOption>
RadioGroup::CreateOption(const string& state_label,
                         const string& option_name) {
  auto option = New<RadioOption>(shared_from_this(),
                                 state_label,
                                 option_name);
  options_.push_back(option.get());
  return option;
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
      : SimpleCandidate("unfold", 0, 0, ""),
        SwitcherCommand("_fold_options") {
    LoadConfig(config);
  }
  virtual void Apply(Switcher* switcher);
  void Append(const string& label) {
    labels_.push_back(label);
  }
  size_t size() const {
    return labels_.size();
  }
  void Finish();

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

static string FirstCharOf(const string& str) {
  if (str.empty()) {
    return str;
  }
  string first_char;
  const char* start = str.c_str();
  const char* end = start;
  utf8::unchecked::next(end);
  return string(start, end - start);
}

void FoldedOptions::Finish() {
  text_ = prefix_;
  bool first = true;
  for (const auto& label : labels_) {
    if (first) {
      first = false;
    }
    else {
      text_ += separator_;
    }
    text_ += abbreviate_options_ ? FirstCharOf(label) : label;
  }
  text_ += suffix_;
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
  Engine* engine = switcher->attached_engine();
  if (!engine)
    return;
  Config* config = engine->schema()->config();
  if (!config)
    return;
  auto switches = config->GetList("switches");
  if (!switches)
    return;
  Context* context = engine->context();
  for (size_t i = 0; i < switches->size(); ++i) {
    auto item = As<ConfigMap>(switches->GetAt(i));
    if (!item)
      continue;
    auto states = As<ConfigList>(item->Get("states"));
    if (!states)
      continue;
    if (auto option_name = item->GetValue("name")) {
      // toggle
      if (states->size() != 2)
        continue;
      bool current_state = context->get_option(option_name->str());
      Append(New<Switch>(
          states->GetValueAt(current_state)->str(),
          states->GetValueAt(1 - current_state)->str(),
          option_name->str(),
          current_state,
          switcher->IsAutoSave(option_name->str())));
    }
    else if (auto options = As<ConfigList>(item->Get("options"))) {
      // radio
      if (states->size() < 2)
        continue;
      if (states->size() != options->size())
        continue;
      auto group = New<RadioGroup>(context, switcher);
      for (size_t i = 0; i < options->size(); ++i) {
        auto option_name = options->GetValueAt(i);
        auto state_label = states->GetValueAt(i);
        if (!option_name || !state_label)
          continue;
        Append(group->CreateOption(state_label->str(), option_name->str()));
      }
      group->SelectOption(group->GetSelectedOption());
    }
  }
  if (switcher->context()->get_option("_fold_options")) {
    auto folded_options = New<FoldedOptions>(switcher->schema()->config());
    for (auto x : candies_) {
      if (Is<Switch>(x) ||
          (Is<RadioOption>(x) && As<RadioOption>(x)->selected())) {
        folded_options->Append(x->text());
      }
    }
    if (folded_options->size() > 1) {
      folded_options->Finish();
      candies_.clear();
      Append(folded_options);
    }
  }
  DLOG(INFO) << "num switches: " << candies_.size();
}

SwitchTranslator::SwitchTranslator(const Ticket& ticket)
    : Translator(ticket) {
}

an<Translation> SwitchTranslator::Query(const string& input,
                                                const Segment& segment) {
  auto switcher = dynamic_cast<Switcher*>(engine_);
  if (!switcher) {
    return nullptr;
  }
  return New<SwitchTranslation>(switcher);
}

}  // namespace rime
