#ifndef RIME_SWITCHES_H_
#define RIME_SWITCHES_H_

#include <rime/common.h>

namespace rime {

class Config;
class ConfigMap;
class ConfigValue;

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

    bool found() const {
      return bool(the_switch);
    }
  };

  void ForEachOption(function<void (SwitchOption option)> callback);

  SwitchOption OptionByName(const string& option_name);

  an<ConfigMap> ByIndex(size_t switch_index);

  static an<ConfigValue> GetStateLabel(
      an<ConfigMap> the_switch, size_t state_index);
  an<ConfigValue> GetStateLabel(const string& option_name, int state);

 private:
  Config* config_;
};

}  // namespace rime

#endif // RIME_SWITCHES_H_
