// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/key_event.h>

namespace rime {

Engine::Engine() : schema_(new Schema), context_(new Context) {
  EZLOGGERFUNCTRACKER;
}

Engine::~Engine() {
  EZLOGGERFUNCTRACKER;
}

bool Engine::ProcessKeyEvent(const KeyEvent &key_event) {
  EZLOGGERVAR(key_event);
  // TODO(gongchen): logic should be defined in processors
  int ch = key_event.keycode();
  if (ch == XK_Return && IsComposing()) {
    Commit();
    return true;
  }
  if (std::isprint(ch)) {
    context_->PushInput(key_event.keycode());
    return true;
  }
  return false;
}

void Engine::Commit() {
  // TODO(gongchen): echoing...
  sink_(context_->input());
  context_->Clear();
}

bool Engine::IsComposing() const {
  return !context_->input().empty();
}

}  // namespace rime
