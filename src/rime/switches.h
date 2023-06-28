#ifndef RIME_SWITCHES_H_
#define RIME_SWITCHES_H_

#include <rime/common.h>

namespace rime {

class Config;
class ConfigItemRef;
class ConfigMap;
class ConfigValue;

struct StringSlice {
  const char* str;
  size_t length;

  operator string() const {
    return str && length ? string(str, length) : string();
  }
};

class Switches {
 public:
  explicit Switches(Config* config) : config_(config) {}

  enum SwitchType {
    kToggleOption,
    kRadioGroup,
  };

  struct SwitchOption {
    an<ConfigMap> the_switch = nullptr;
    SwitchType type = kToggleOption;
    string option_name;
    // reset state value on initialize. -1 if unspecified.
    int reset_value = -1;
    // index of the switch configuration.
    size_t switch_index = 0;
    // the index of the option in the radio group.
    size_t option_index = 0;

    bool found() const { return bool(the_switch); }
  };

  enum FindResult {
    kContinue,
    kFound,
  };

  SwitchOption FindOption(function<FindResult(SwitchOption option)> callback);

  SwitchOption OptionByName(const string& option_name);

  // Returns the switch option defined at switch_index.
  // If the swtich is a radio group, return the first option in the group.
  SwitchOption ByIndex(size_t switch_index);

  static SwitchOption Cycle(const SwitchOption& option);

  static SwitchOption Reset(const SwitchOption& option);

  static SwitchOption FindRadioGroupOption(
      an<ConfigMap> the_switch,
      function<FindResult(SwitchOption option)> callback);

  static StringSlice GetStateLabel(an<ConfigMap> the_switch,
                                   size_t state_index,
                                   bool abbreviated);

  StringSlice GetStateLabel(const string& option_name,
                            int state,
                            bool abbreviated);

 private:
  SwitchOption FindOptionFromConfigItem(
      ConfigItemRef& item,
      size_t switch_index,
      function<FindResult(SwitchOption option)> callback);

  Config* config_;
};

}  // namespace rime

#endif  // RIME_SWITCHES_H_
