#include <rime/config.h>
#include <rime/switches.h>

namespace rime {

void Switches::ForEachOption(function<void (SwitchOption option)> callback) {
  auto switches = (*config_)["switches"];
  if (!switches.IsList())
    return;
  for (size_t i = 0; i < switches.size(); ++i) {
    auto item = switches[i];
    if (!item.IsMap())
      continue;
    auto reset = item["reset"];
    int reset_value = reset.IsValue() ? reset.ToInt() : -1;
    auto name = item["name"];
    if (name.IsValue()) {
      callback(
          SwitchOption{
            As<ConfigMap>(*item),
            kToggleOption,
            name.ToString(),
            reset_value,
            i
          });
      continue;
    }
    auto options = item["options"];
    if (options.IsList()) {
      for (size_t j = 0; j < options.size(); ++j) {
        callback(
            SwitchOption{
              As<ConfigMap>(*item),
              kRadioGroup,
              options[j].ToString(),
              reset_value,
              i,
              j
            });
      }
    }
  }
}

Switches::SwitchOption Switches::OptionByName(const string& option_name) {
  ForEachOption([&option_name](SwitchOption option) {
    if (option.option_name == option_name)
      return option;
  });
  return {};
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
