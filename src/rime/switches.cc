#include <rime/config.h>
#include <rime/switches.h>

namespace rime {

Switches::SwitchOption Switches::FindOption(
    function<FindResult (SwitchOption option)> callback) {
  auto switches = (*config_)["switches"];
  if (!switches.IsList())
    return {};
  for (size_t i = 0; i < switches.size(); ++i) {
    auto item = switches[i];
    if (!item.IsMap())
      continue;
    auto the_switch = As<ConfigMap>(*item);
    auto reset = item["reset"];
    int reset_value = reset.IsValue() ? reset.ToInt() : -1;
    auto name = item["name"];
    if (name.IsValue()) {
      SwitchOption option{
        the_switch,
        kToggleOption,
        name.ToString(),
        reset_value,
        i,
      };
      if (callback(option) == kFound)
        return option;
      continue;
    }
    auto options = item["options"];
    if (options.IsList()) {
      for (size_t j = 0; j < options.size(); ++j) {
        SwitchOption option{
          the_switch,
          kRadioGroup,
          options[j].ToString(),
          reset_value,
          i,
          j,
        };
        if (callback(option) == kFound)
          return option;
      }
    }
  }
  return {};
}

Switches::SwitchOption Switches::OptionByName(const string& option_name) {
  return FindOption([&option_name](SwitchOption option) {
    return option.option_name == option_name ? kFound : kContinue;
  });
}

an<ConfigMap> Switches::ByIndex(size_t switch_index) {
  auto switches = (*config_)["switches"];
  if (!switches.IsList())
    return nullptr;
  if (switches.size() <= switch_index)
    return nullptr;
  auto item = switches[switch_index];
  return As<ConfigMap>(*item);
}

Switches::SwitchOption Switches::Cycle(const SwitchOption& current) {
  if (auto options = As<ConfigList>(current.the_switch->Get("options"))) {
    size_t next_option_index = (current.option_index + 1) % options->size();
    if (next_option_index != current.option_index) {
      return {
        current.the_switch,
        current.type,
        options->GetValueAt(next_option_index)->str(),
        current.reset_value,
        current.switch_index,
        next_option_index,
      };
    }
  }
  return {};
}

Switches::SwitchOption Switches::Reset(const SwitchOption& current) {
  size_t default_state = (current.reset_value >= 0) ? current.reset_value : 0;
  if (auto options = As<ConfigList>(current.the_switch->Get("options"))) {
    if (default_state >= options->size() ||
        default_state == current.option_index)
      return {};
    return {
      current.the_switch,
      current.type,
      options->GetValueAt(default_state)->str(),
      current.reset_value,
      current.switch_index,
      default_state,
    };
  }
}

Switches::SwitchOption Switches::FindRadioGroupOption(
    an<ConfigMap> the_switch,
    function<FindResult (SwitchOption option)> callback) {
  if (auto options = As<ConfigList>(the_switch->Get("options"))) {
    for (size_t j = 0; j < options->size(); ++j) {
      SwitchOption option{
        the_switch,
        kRadioGroup,
        options->GetValueAt(j)->str(),
        0,  // unknown
        0,  // unknown
        j,
      };
      if (callback(option) == kFound)
        return option;
    }
  }
  return {};
}

an<ConfigValue> Switches::GetStateLabel(an<ConfigMap> the_switch,
                                        size_t state_index) {
  if (!the_switch)
    return nullptr;
  auto states = As<ConfigList>(the_switch->Get("states"));
  if (!states)
    return nullptr;
  return states->GetValueAt(state_index);
}

an<ConfigValue> Switches::GetStateLabel(const string& option_name, int state) {
  auto the_option = OptionByName(option_name);
  if (!the_option.found())
    return nullptr;
  if (the_option.type == kToggleOption) {
    size_t state_index = static_cast<size_t>(state);
    return GetStateLabel(the_option.the_switch, state_index);
  }
  if (the_option.type == kRadioGroup) {
    // if the query is a deselected option among the radio group, do not
    // display its state label; only show the selected option.
    return state
        ? GetStateLabel(the_option.the_switch, the_option.option_index)
        : nullptr;
  }
  return nullptr;
}

}  // namespace rime
