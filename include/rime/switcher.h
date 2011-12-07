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

#include <rime/common.h>
#include <rime/config.h>

namespace rime {

class KeyEvent;
class Schema;
class Context;

class Switcher {
 public:
  Switcher();
  ~Switcher();
  
  bool ProcessKeyEvent(const KeyEvent &key_event);
  Schema* CreateSchema();

  Context* context() const { return context_.get(); }
  bool active() const { return active_; }
  
 protected:
  void OnSelect(Context *ctx);
  
  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  bool active_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_H_
