//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SWITCHER_H_
#define RIME_SWITCHER_H_

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

  virtual bool ProcessKey(const KeyEvent& key_event) {
    return ProcessKeyEvent(key_event) == kAccepted;
  }
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

  Schema* CreateSchema();
  void SelectNextSchema();
  bool IsAutoSave(const string& option) const;

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

  the<Config> user_config_;
  string caption_;
  vector<KeyEvent> hotkeys_;
  set<string> save_options_;
  bool fold_options_ = false;

  vector<of<Processor>> processors_;
  vector<of<Translator>> translators_;
  bool active_ = false;
};

class SwitcherCommand {
 public:
  SwitcherCommand(const string& keyword)
      : keyword_(keyword) {
  }
  virtual void Apply(Switcher* switcher) = 0;
  const string& keyword() const { return keyword_; }

 protected:
  string keyword_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_H_
