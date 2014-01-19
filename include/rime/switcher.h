//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SWITCHER_H_
#define RIME_SWITCHER_H_

#include <set>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/engine.h>
#include <rime/key_event.h>

namespace rime {

class Config;
class Processor;
class Translator;

class Switcher : public Engine {
 public:
  Switcher();
  ~Switcher();

  bool ProcessKeyEvent(const KeyEvent &key_event);
  void Attach(Engine *engine);
  void ApplySchema(Schema* schema);

  Schema* CreateSchema();
  void SelectNextSchema();
  bool IsAutoSave(const std::string& option) const;

  Config* user_config() const { return user_config_.get(); }
  bool active() const { return active_; }

 protected:
  void InitializeComponents();
  void LoadSettings();
  void Activate();
  void Deactivate();
  void HighlightNextSchema();
  void OnSelect(Context *ctx);

  scoped_ptr<Config> user_config_;
  std::string caption_;
  std::vector<KeyEvent> hotkeys_;
  std::set<std::string> save_options_;
  std::vector<shared_ptr<Processor> > processors_;
  std::vector<shared_ptr<Translator> > translators_;
  bool active_;
};

class SwitcherCommand {
 public:
  SwitcherCommand(const std::string& keyword)
      : keyword_(keyword) {
  }
  virtual void Apply(Switcher* switcher) = 0;

 protected:
  std::string keyword_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_H_
