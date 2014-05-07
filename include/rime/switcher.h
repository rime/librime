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
#include <rime/processor.h>

namespace rime {

class Config;
class Context;
class Translator;

class Switcher : public Processor, public Engine {
 public:
  Switcher(const Ticket& ticket);
  virtual ~Switcher();

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

  Schema* CreateSchema();
  void SelectNextSchema();
  bool IsAutoSave(const std::string& option) const;

  void RefreshMenu();
  void Activate();
  void Deactivate();

  Engine* attached_engine() const { return engine_; }
  Config* user_config() const { return user_config_.get(); }
  bool active() const { return active_; }

 protected:
  void InitializeComponents();
  void LoadSettings();
  void RestoreSavedOptions();
  void HighlightNextSchema();
  void OnSelect(Context* ctx);

  unique_ptr<Config> user_config_;
  std::string caption_;
  std::vector<KeyEvent> hotkeys_;
  std::set<std::string> save_options_;
  bool fold_options_ = false;

  std::vector<shared_ptr<Processor>> processors_;
  std::vector<shared_ptr<Translator>> translators_;
  bool active_ = false;
};

class SwitcherCommand {
 public:
  SwitcherCommand(const std::string& keyword)
      : keyword_(keyword) {
  }
  virtual void Apply(Switcher* switcher) = 0;
  const std::string& keyword() const { return keyword_; }

 protected:
  std::string keyword_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_H_
