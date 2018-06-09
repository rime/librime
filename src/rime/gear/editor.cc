//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/key_table.h>
#include <rime/gear/editor.h>
#include <rime/gear/key_binding_processor.h>
#include <rime/gear/translator_commons.h>

namespace rime {

static Editor::ActionDef editor_action_definitions[] = {
  { "confirm",  &Editor::Confirm },
  { "toggle_selection",  &Editor::ToggleSelection },
  { "commit_comment", &Editor::CommitComment },
  { "commit_raw_input", &Editor::CommitRawInput },
  { "commit_script_text", &Editor::CommitScriptText },
  { "commit_composition", &Editor::CommitComposition },
  { "revert", &Editor::RevertLastEdit },
  { "back", &Editor::BackToPreviousInput },
  { "back_syllable", &Editor::BackToPreviousSyllable },
  { "delete_candidate", &Editor::DeleteCandidate },
  { "delete", &Editor::DeleteChar },
  { "cancel", &Editor::CancelComposition },
  Editor::kActionNoop
};

static struct EditorCharHandlerDef {
  const char* name;
  Editor::CharHandlerPtr action;
} editor_char_handler_definitions[] = {
  { "direct_commit", &Editor::DirectCommit },
  { "add_to_input", &Editor::AddToInput },
  { "noop", nullptr }
};

Editor::Editor(const Ticket& ticket, bool auto_commit)
    : Processor(ticket), KeyBindingProcessor(editor_action_definitions) {
  engine_->context()->set_option("_auto_commit", auto_commit);
}

ProcessResult Editor::ProcessKeyEvent(const KeyEvent& key_event) {
  if (key_event.release())
    return kRejected;
  int ch = key_event.keycode();
  Context* ctx = engine_->context();
  if (ctx->IsComposing()) {
    auto result = KeyBindingProcessor::ProcessKeyEvent(key_event, ctx);
    if (result != kNoop) {
      return result;
    }
  }
  if (char_handler_ &&
      !key_event.ctrl() && !key_event.alt() &&
      ch > 0x20 && ch < 0x7f) {
    DLOG(INFO) << "input char: '" << (char)ch << "', " << ch
               << ", '" << key_event.repr() << "'";
    return RIME_THIS_CALL(char_handler_)(ctx, ch);
  }
  // not handled
  return kNoop;
}

void Editor::LoadConfig() {
  if (!engine_) {
    return;
  }
  Config* config = engine_->schema()->config();
  KeyBindingProcessor::LoadConfig(config, "editor");
  if (auto value = config->GetValue("editor/char_handler")) {
    auto* p = editor_char_handler_definitions;
    while (p->action && p->name != value->str()) {
      ++p;
    }
    if (!p->action && p->name != value->str()) {
      LOG(WARNING) << "invalid char_handler: " << value->str();
    }
    else {
      char_handler_ = p->action;
    }
  }
}

void Editor::Confirm(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
}

void Editor::ToggleSelection(Context* ctx) {
  ctx->ReopenPreviousSegment() ||
      ctx->ConfirmCurrentSelection();
}

void Editor::CommitComment(Context* ctx) {
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (!cand->comment().empty()) {
      engine_->sink()(cand->comment());
      ctx->Clear();
    }
  }
}

void Editor::CommitScriptText(Context* ctx) {
  engine_->sink()(ctx->GetScriptText());
  ctx->Clear();
}

void Editor::CommitRawInput(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
}

void Editor::CommitComposition(Context* ctx) {
  if (!ctx->ConfirmCurrentSelection() || !ctx->HasMenu())
    ctx->Commit();
}

void Editor::RevertLastEdit(Context* ctx) {
  // different behavior in regard to previous operation type
  ctx->ReopenPreviousSelection() ||
      (ctx->PopInput() && ctx->ReopenPreviousSegment());
}

void Editor::BackToPreviousInput(Context* ctx) {
  ctx->ReopenPreviousSegment() ||
      ctx->ReopenPreviousSelection() ||
      ctx->PopInput();
}

void Editor::BackToPreviousSyllable(Context* ctx) {
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return;
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(cand))) {
      size_t stop = phrase->spans().PreviousStop(caret_pos);
      if (stop != caret_pos) {
        ctx->PopInput(caret_pos - stop);
        return;
      }
    }
  }
  ctx->PopInput();
}

void Editor::DeleteCandidate(Context* ctx) {
  ctx->DeleteCurrentSelection();
}

void Editor::DeleteChar(Context* ctx) {
  ctx->DeleteInput();
}

void Editor::CancelComposition(Context* ctx) {
  if (!ctx->ClearPreviousSegment())
    ctx->Clear();
}

ProcessResult Editor::DirectCommit(Context* ctx, int ch) {
  ctx->Commit();
  return kRejected;
}

ProcessResult Editor::AddToInput(Context* ctx, int ch) {
    ctx->PushInput(ch);
    ctx->ConfirmPreviousSelection();
    return kAccepted;
}

FluidEditor::FluidEditor(const Ticket& ticket) : Editor(ticket, false) {
  Bind({XK_space, 0}, &Editor::Confirm);
  Bind({XK_BackSpace, 0}, &Editor::BackToPreviousInput);  //
  Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  Bind({XK_Return, 0}, &Editor::CommitComposition);  //
  Bind({XK_Return, kControlMask}, &Editor::CommitRawInput);  //
  Bind({XK_Return, kShiftMask}, &Editor::CommitScriptText);  //
  Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  Bind({XK_Delete, 0}, &Editor::DeleteChar);
  Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::AddToInput;  //
  LoadConfig();
}

ExpressEditor::ExpressEditor(const Ticket& ticket) : Editor(ticket, true) {
  Bind({XK_space, 0}, &Editor::Confirm);
  Bind({XK_BackSpace, 0}, &Editor::RevertLastEdit);  //
  Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  Bind({XK_Return, 0}, &Editor::CommitRawInput);  //
  Bind({XK_Return, kControlMask}, &Editor::CommitScriptText);  //
  Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  Bind({XK_Delete, 0}, &Editor::DeleteChar);
  Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::DirectCommit;  //
  LoadConfig();
}

}  // namespace rime
