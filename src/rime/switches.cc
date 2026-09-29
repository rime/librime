#include <utf8.h>
#include <rime/config.h>
#include <rime/switches.h>

namespace rime {

inline static int reset_value(ConfigItemRef& item) {
  auto reset = item["reset"];
  return reset.IsValue() ? reset.ToInt() : -1;
}

Switches::SwitchOption Switches::FindOptionFromConfigItem(
    ConfigItemRef& item,
    size_t switch_index,
    function<FindResult(SwitchOption option)> callback) {
  auto the_switch = As<ConfigMap>(*item);
  auto name = item["name"];
  auto options = item["options"];
  if (name.IsValue()) {
    SwitchOption option{
        the_switch,        kToggleOption, name.ToString(),
        reset_value(item), switch_index,
    };
    if (callback(option) == kFound)
      return option;
  } else if (options.IsList()) {
    for (size_t option_index = 0; option_index < options.size();
         ++option_index) {
      SwitchOption option{
          the_switch,        kRadioGroup,  options[option_index].ToString(),
          reset_value(item), switch_index, option_index,
      };
      if (callback(option) == kFound)
        return option;
    }
  }
  return {};
}

Switches::SwitchOption Switches::FindOption(
    function<FindResult(SwitchOption option)> callback) {
  auto switches = (*config_)["switches"];
  if (!switches.IsList())
    return {};
  for (size_t switch_index = 0; switch_index < switches.size();
       ++switch_index) {
    auto item = switches[switch_index];
    if (!item.IsMap())
      continue;
    auto option = FindOptionFromConfigItem(item, switch_index, callback);
    if (option.found())
      return option;
  }
  return {};
}

Switches::SwitchOption Switches::OptionByName(const string& option_name) {
  return FindOption([&option_name](SwitchOption option) {
    return option.option_name == option_name ? kFound : kContinue;
  });
}

Switches::SwitchOption Switches::ByIndex(size_t switch_index) {
  auto switches = (*config_)["switches"];
  if (!switches.IsList())
    return {};
  if (switches.size() <= switch_index)
    return {};
  auto item = switches[switch_index];
  return FindOptionFromConfigItem(item, switch_index,
                                  // return the very first found option.
                                  [](SwitchOption option) { return kFound; });
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
  return {};
}

Switches::SwitchOption Switches::FindRadioGroupOption(
    an<ConfigMap> the_switch,
    function<FindResult(SwitchOption option)> callback) {
  if (auto options = As<ConfigList>(the_switch->Get("options"))) {
    for (size_t j = 0; j < options->size(); ++j) {
      SwitchOption option{
          the_switch, kRadioGroup, options->GetValueAt(j)->str(),
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

inline static size_t first_unicode_byte_length(const string& str) {
  if (str.empty()) {
    return 0;
  }
  const char* start = str.c_str();
  const char* end = start;
  utf8::unchecked::next(end);
  return end - start;
}

StringSlice Switches::GetStateLabel(an<ConfigMap> the_switch,
                                    size_t state_index,
                                    bool abbreviated) {
  if (!the_switch)
    return {nullptr, 0};
  auto states = As<ConfigList>(the_switch->Get("states"));
  if (!states || states->size() <= state_index) {
    return {nullptr, 0};
  }
  if (abbreviated) {
    auto abbrev = As<ConfigList>(the_switch->Get("abbrev"));
    if (abbrev && abbrev->size() > state_index) {
      auto value = abbrev->GetValueAt(state_index);
      return {value->str().c_str(), value->str().length()};
    } else {
      auto value = states->GetValueAt(state_index);
      return {value->str().c_str(), first_unicode_byte_length(value->str())};
    }
  } else {
    auto value = states->GetValueAt(state_index);
    return {value->str().c_str(), value->str().length()};
  }
}

StringSlice Switches::GetStateLabel(const string& option_name,
                                    int state,
                                    bool abbreviated) {
  auto the_option = OptionByName(option_name);
  if (!the_option.found()) {
    return {nullptr, 0};
  }
  if (the_option.type == kToggleOption) {
    size_t state_index = static_cast<size_t>(state);
    return GetStateLabel(the_option.the_switch, state_index, abbreviated);
  }
  if (the_option.type == kRadioGroup) {
    // if the query is a deselected option among the radio group, do not
    // display its state label; only show the selected option.
    return state ? GetStateLabel(the_option.the_switch, the_option.option_index,
                                 abbreviated)
                 : StringSlice{nullptr, 0};
  }
  return {nullptr, 0};
}

}  // namespace rime
