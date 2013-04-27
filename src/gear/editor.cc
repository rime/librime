//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/gear/editor.h>
#include <rime/gear/translator_commons.h>

namespace rime {

Editor::Editor(Engine* engine, bool auto_commit) : Processor(engine) {
  engine->context()->set_option("_auto_commit", auto_commit);
}

Processor::Result Editor::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release() || key_event.alt())
    return kRejected;
  int ch = key_event.keycode();
  if (key_event.ctrl() && !WorkWithCtrl(ch))
    return kNoop;
  Context *ctx = engine_->context();
  if (ch == XK_space) {
    if (ctx->IsComposing()) {
      OnSpace(ctx);
      return kAccepted;
    }
    else {
      return kNoop;
    }
  }
  if (ctx->IsComposing()) {
    if (ch == XK_Return) {
      if (key_event.shift() || key_event.ctrl()) {
	OnShiftReturn(ctx);
      }
      else {
	OnReturn(ctx);
      }
      return kAccepted;
    }
    if (ch == XK_BackSpace) {
      if (key_event.shift() || key_event.ctrl()) {
        OnShiftBackSpace(ctx);
      }
      else {
        OnBackSpace(ctx);
      }
      return kAccepted;
    }
    if (ch == XK_Delete || ch == XK_KP_Delete) {
      if (key_event.shift() || key_event.ctrl()) {
	OnShiftDelete(ctx);
      }
      else {
	OnDelete(ctx);
      }
      return kAccepted;
    }
    if (ch == XK_Escape) {
      OnEscape(ctx);
      return kAccepted;
    }
  }
  if (ch > 0x20 && ch < 0x7f) {
    DLOG(INFO) << "input char: '" << (char)ch << "', " << ch
               << ", '" << key_event.repr() << "'";
    return OnChar(ctx, ch);
  }
  // not handled
  return kNoop;
}

inline bool Editor::WorkWithCtrl(int ch) {
  return ch == XK_Return || ch == XK_BackSpace ||
    ch == XK_Delete || ch == XK_KP_Delete;
}

inline void Editor::Confirm(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
}

inline void Editor::CommitScriptText(Context* ctx) {
  engine_->sink()(ctx->GetScriptText());
  ctx->Clear();
}

inline void Editor::CommitRawInput(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
}

inline void Editor::CommitComposition(Context* ctx) {
  if (!ctx->ConfirmCurrentSelection() || !ctx->HasMenu())
    ctx->Commit();
}

inline void Editor::RevertLastAction(Context* ctx) {
  // different behavior in regard to previous operation type
  ctx->ReopenPreviousSelection() ||
    ctx->PopInput() && ctx->ReopenPreviousSegment();
}

inline void Editor::RevertToPreviousInput(Context* ctx) {
  ctx->ReopenPreviousSegment() ||
      ctx->ReopenPreviousSelection() ||
      ctx->PopInput();
}

void Editor::DropPreviousSyllable(Context* ctx) {
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return;
  const Composition* comp = ctx->composition();
  if (comp && !comp->empty()) {
    shared_ptr<Candidate> cand = comp->back().GetSelectedCandidate();
    shared_ptr<Phrase> phrase =
        As<Phrase>(Candidate::GetGenuineCandidate(cand));
    if (phrase && phrase->syllabification()) {
      size_t stop = phrase->syllabification()->PreviousStop(caret_pos);
      if (stop != caret_pos) {
        ctx->PopInput(caret_pos - stop);
        return;
      }
    }
  }
  ctx->PopInput();
}

inline void Editor::DeleteHighlightedPhrase(Context* ctx) {
  ctx->DeleteCurrentSelection();
}

inline void Editor::DeleteChar(Context* ctx) {
  ctx->DeleteInput();
}

inline void Editor::CancelComposition(Context* ctx) {
  if (!ctx->ClearPreviousSegment())
    ctx->Clear();
}

inline Processor::Result Editor::DirectCommit(Context* ctx, int ch) {
  ctx->Commit();
  return kRejected;
}

inline Processor::Result Editor::AddToInput(Context* ctx, int ch) {
    ctx->PushInput(ch);
    ctx->ConfirmPreviousSelection();
    return kAccepted;
}

inline void Editor::OnSpace(Context* ctx) {
  Confirm(ctx);
}

inline void Editor::OnBackSpace(Context* ctx) {
  RevertToPreviousInput(ctx);
}

inline void Editor::OnShiftBackSpace(Context* ctx) {
  DropPreviousSyllable(ctx);
}

inline void Editor::OnReturn(Context* ctx) {
  CommitComposition(ctx);
}

inline void Editor::OnShiftReturn(Context* ctx) {
  CommitScriptText(ctx);
}

inline void Editor::OnDelete(Context* ctx) {
  DeleteChar(ctx);
}

inline void Editor::OnShiftDelete(Context* ctx) {
  DeleteHighlightedPhrase(ctx);
}

inline void Editor::OnEscape(Context* ctx) {
  CancelComposition(ctx);
}

inline Processor::Result Editor::OnChar(Context* ctx, int ch) {
  return AddToInput(ctx, ch);
}

//

FluencyEditor::FluencyEditor(Engine* engine) : Editor(engine, false) {
}

ExpressEditor::ExpressEditor(Engine* engine) : Editor(engine, true) {
}

inline void ExpressEditor::OnBackSpace(Context* ctx) {
  RevertLastAction(ctx);
}

inline void ExpressEditor::OnReturn(Context* ctx) {
  CommitRawInput(ctx);
}

inline Processor::Result ExpressEditor::OnChar(Context* ctx, int ch) {
  return DirectCommit(ctx, ch);
}

}  // namespace rime
