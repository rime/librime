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
    {"confirm", &Editor::Confirm},
    {"toggle_selection", &Editor::ToggleSelection},
    {"commit_comment", &Editor::CommitComment},
    {"commit_raw_input", &Editor::CommitRawInput},
    {"commit_script_text", &Editor::CommitScriptText},
    {"commit_composition", &Editor::CommitComposition},
    {"revert", &Editor::RevertLastEdit},
    {"back", &Editor::BackToPreviousInput},
    {"back_syllable", &Editor::BackToPreviousSyllable},
    {"delete_candidate", &Editor::DeleteCandidate},
    {"delete", &Editor::DeleteChar},
    {"cancel", &Editor::CancelComposition},
    Editor::kActionNoop};

static struct EditorCharHandlerDef {
  const char* name;
  Editor::CharHandlerPtr action;
} editor_char_handler_definitions[] = {{"direct_commit", &Editor::DirectCommit},
                                       {"add_to_input", &Editor::AddToInput},
                                       {"noop", nullptr}};

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
    auto result = KeyBindingProcessor::ProcessKeyEvent(key_event, ctx, 0,
                                                       FallbackOptions::All);
    if (result != kNoop) {
      return result;
    }
  }
  if (char_handler_ && !key_event.ctrl() && !key_event.alt() &&
      !key_event.super() && ch > 0x20 && ch < 0x7f) {
    DLOG(INFO) << "input char: '" << (char)ch << "', " << ch << ", '"
               << key_event.repr() << "'";
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
    } else {
      char_handler_ = p->action;
    }
  }
}

bool Editor::Confirm(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
  return true;
}

bool Editor::ToggleSelection(Context* ctx) {
  ctx->ReopenPreviousSegment() || ctx->ConfirmCurrentSelection();
  return true;
}

bool Editor::CommitComment(Context* ctx) {
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (!cand->comment().empty()) {
      engine_->sink()(cand->comment());
      ctx->Clear();
    }
  }
  return true;
}

bool Editor::CommitScriptText(Context* ctx) {
  engine_->sink()(ctx->GetScriptText());
  ctx->Clear();
  return true;
}

bool Editor::CommitRawInput(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
  return true;
}

bool Editor::CommitComposition(Context* ctx) {
  if (!ctx->ConfirmCurrentSelection() || !ctx->HasMenu())
    ctx->Commit();
  return true;
}

bool Editor::RevertLastEdit(Context* ctx) {
  // different behavior in regard to previous operation type
  ctx->ReopenPreviousSelection() ||
      (ctx->PopInput() && ctx->ReopenPreviousSegment());
  return true;
}

bool Editor::BackToPreviousInput(Context* ctx) {
  ctx->ReopenPreviousSegment() || ctx->ReopenPreviousSelection() ||
      ctx->PopInput();
  return true;
}

static bool pop_input_by_syllable(Context* ctx) {
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return false;
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(cand))) {
      size_t stop = phrase->spans().PreviousStop(caret_pos);
      if (stop != caret_pos) {
        ctx->PopInput(caret_pos - stop);
        return true;
      }
    }
  }
  return false;
}

bool Editor::BackToPreviousSyllable(Context* ctx) {
  ctx->ReopenPreviousSelection() ||
      ((pop_input_by_syllable(ctx) || ctx->PopInput()) &&
       ctx->ReopenPreviousSegment());
  return true;
}

bool Editor::DeleteCandidate(Context* ctx) {
  ctx->DeleteCurrentSelection();
  return true;
}

bool Editor::DeleteChar(Context* ctx) {
  ctx->DeleteInput();
  return true;
}

bool Editor::CancelComposition(Context* ctx) {
  if (!ctx->ClearPreviousSegment())
    ctx->Clear();
  return true;
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
  auto& keymap = get_keymap();
  keymap.Bind({XK_space, 0}, &Editor::Confirm);
  keymap.Bind({XK_BackSpace, 0}, &Editor::BackToPreviousInput);  //
  keymap.Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  keymap.Bind({XK_Return, 0}, &Editor::CommitComposition);          //
  keymap.Bind({XK_Return, kControlMask}, &Editor::CommitRawInput);  //
  keymap.Bind({XK_Return, kShiftMask}, &Editor::CommitScriptText);  //
  keymap.Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  keymap.Bind({XK_Delete, 0}, &Editor::DeleteChar);
  keymap.Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  keymap.Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::AddToInput;  //
  LoadConfig();
}

ExpressEditor::ExpressEditor(const Ticket& ticket) : Editor(ticket, true) {
  auto& keymap = get_keymap();
  keymap.Bind({XK_space, 0}, &Editor::Confirm);
  keymap.Bind({XK_BackSpace, 0}, &Editor::RevertLastEdit);  //
  keymap.Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  keymap.Bind({XK_Return, 0}, &Editor::CommitRawInput);               //
  keymap.Bind({XK_Return, kControlMask}, &Editor::CommitScriptText);  //
  keymap.Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  keymap.Bind({XK_Delete, 0}, &Editor::DeleteChar);
  keymap.Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  keymap.Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::DirectCommit;  //
  LoadConfig();
}

}  // namespace rime
