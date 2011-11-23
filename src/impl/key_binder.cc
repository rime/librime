// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-23 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/impl/key_binder.h>

namespace rime {

KeyBinder::KeyBinder(Engine *engine) : Processor(engine) {
}

Processor::Result KeyBinder::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release())
    return kNoop;
  Context *ctx = engine_->context();
  // not handled
  return kNoop;
}

}  // namespace rime
