// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_EDITOR_H_
#define RIME_EDITOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Context;

class Editor : public Processor {
 public:
  typedef void Handler(Context* ctx);
  typedef Result CharHandler(Context* ctx, int ch);
  
  Editor(Engine* engine, bool auto_commit);
  Result ProcessKeyEvent(const KeyEvent &key_event);
  
  virtual Handler OnSpace;
  virtual Handler OnBackSpace;
  virtual Handler OnReturn;
  Handler OnShiftReturn;
  Handler OnDelete;
  Handler OnShiftDelete;
  Handler OnEscape;
  virtual CharHandler OnChar;

 protected:
  bool WorkWithCtrl(int ch);
  Handler Confirm;
  Handler CommitScriptText;
  Handler CommitRawInput;
  Handler CommitComposition;
  Handler RevertLastAction;
  Handler DiscoverPreviousInput;
  Handler DeleteHighlightedPhrase;
  Handler DeleteChar;
  Handler CancelComposition;
  CharHandler DirectCommit;
  CharHandler AddToInput;
};

class FluencyEditor : public Editor {
 public:
  FluencyEditor(Engine* engine);
};

class ExpressEditor : public Editor {
 public:
  ExpressEditor(Engine* engine);
  Handler OnBackSpace;
  Handler OnReturn;
  CharHandler OnChar;
};

}  // namespace rime

#endif  // RIME_EDITOR_H_
