//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_EDITOR_H_
#define RIME_EDITOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/key_event.h>
#include <rime/processor.h>

namespace rime {

class Context;

class Editor : public Processor {
 public:
  typedef void Handler(Context* ctx);
  typedef ProcessResult CharHandler(Context* ctx, int ch);
  using HandlerPtr = void (Editor::*)(Context* ctx);
  using CharHandlerPtr = ProcessResult (Editor::*)(Context* ctx, int ch);
  using KeyBindings = map<KeyEvent, HandlerPtr>;

  Editor(const Ticket& ticket, bool auto_commit);
  ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

  Handler Confirm;
  Handler ToggleSelection;
  Handler CommitComment;
  Handler CommitScriptText;
  Handler CommitRawInput;
  Handler CommitComposition;
  Handler RevertLastEdit;
  Handler BackToPreviousInput;
  Handler BackToPreviousSyllable;
  Handler DeleteCandidate;
  Handler DeleteChar;
  Handler CancelComposition;

  CharHandler DirectCommit;
  CharHandler AddToInput;

 protected:
  bool Accept(const KeyEvent& key_event);
  void Bind(KeyEvent key_event, HandlerPtr action);
  void LoadConfig();

  KeyBindings key_bindings_;
  CharHandlerPtr char_handler_ = nullptr;
};

class FluidEditor : public Editor {
 public:
  FluidEditor(const Ticket& ticket);
};

class ExpressEditor : public Editor {
 public:
  ExpressEditor(const Ticket& ticket);
};

}  // namespace rime

#endif  // RIME_EDITOR_H_
