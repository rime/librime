// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SWITCHER_H_
#define RIME_SWITCHER_H_

#include <vector>
#include <rime/common.h>
#include <rime/engine.h>
#include <rime/key_event.h>

namespace rime {

class Processor;

class Switcher : public Engine {
 public:
  Switcher();
  ~Switcher();
  
  void Attach(Engine *engine);
  bool ProcessKeyEvent(const KeyEvent &key_event);
  Schema* CreateSchema();

  Context* context() const { return context_.get(); }
  bool active() const { return active_; }
  
 protected:
  void InitializeSubProcessors();
  void LoadSettings();
  void Activate();
  void Deactivate();
  void OnSelect(Context *ctx);

  std::string caption_;
  std::vector<KeyEvent> hotkeys_;
  std::vector<shared_ptr<Processor> > processors_;
  Engine *target_engine_;
  bool active_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_H_
