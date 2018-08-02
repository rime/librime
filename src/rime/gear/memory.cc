//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-02 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/composition.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/language.h>
#include <rime/schema.h>
#include <rime/ticket.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/memory.h>
#include <rime/gear/translator_commons.h>

namespace rime {

void CommitEntry::Clear() {
  text.clear();
  code.clear();
  elements.clear();
}

void CommitEntry::AppendPhrase(const an<Phrase>& phrase) {
  text += phrase->text();
  code.insert(code.end(),
              phrase->code().begin(), phrase->code().end());
  if (auto sentence = As<Sentence>(phrase)) {
    for (const DictEntry& e : sentence->components()) {
      elements.push_back(&e);
    }
  }
  else {
    elements.push_back(&phrase->entry());
  }
}

bool CommitEntry::Save() const {
  if (memory && !empty()) {
    DLOG(INFO) << "memorize commit entry: " << text;
    return memory->Memorize(*this);
  }
  return false;
}

Memory::Memory(const Ticket& ticket) {
  if (!ticket.engine)
    return;

  if (auto dictionary = Dictionary::Require("dictionary")) {
    dict_.reset(dictionary->Create(ticket));
    if (dict_)
      dict_->Load();
  }

  if (auto user_dictionary = UserDictionary::Require("user_dictionary")) {
    user_dict_.reset(user_dictionary->Create(ticket));
    if (user_dict_) {
      user_dict_->Load();
      if (dict_)
        user_dict_->Attach(dict_->table(), dict_->prism());
    }
  }

  // user dictionary is named after language; dictionary name may have an
  // optional suffix separated from the language component by dot.
  language_.reset(new Language{
    user_dict_ ? user_dict_->name() :
    Language::get_language_component(dict_->name())
  });

  Context* ctx = ticket.engine->context();
  commit_connection_ = ctx->commit_notifier().connect(
      [this](Context* ctx) { OnCommit(ctx); });
  delete_connection_ = ctx->delete_notifier().connect(
      [this](Context* ctx) { OnDeleteEntry(ctx); });
  unhandled_key_connection_ = ctx->unhandled_key_notifier().connect(
      [this](Context* ctx, const KeyEvent& key) { OnUnhandledKey(ctx, key); });
}

Memory::~Memory() {
  commit_connection_.disconnect();
  delete_connection_.disconnect();
  unhandled_key_connection_.disconnect();
}

bool Memory::StartSession() {
  return user_dict_ && user_dict_->NewTransaction();
}

bool Memory::FinishSession() {
  return user_dict_ && user_dict_->CommitPendingTransaction();
}

bool Memory::DiscardSession() {
  return user_dict_ && user_dict_->RevertRecentTransaction();
}

void Memory::OnCommit(Context* ctx) {
  if (!user_dict_|| user_dict_->readonly())
    return;
  StartSession();
  CommitEntry commit_entry(this);
  for (auto& seg : ctx->composition()) {
    auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(
            seg.GetSelectedCandidate()));
    bool recognized = Language::intelligible(phrase, this);
    if (recognized) {
      commit_entry.AppendPhrase(phrase);
    }
    if (!recognized || seg.status >= Segment::kConfirmed) {
      commit_entry.Save();
      commit_entry.Clear();
    }
  }
}

void Memory::OnDeleteEntry(Context* ctx) {
  if (!user_dict_ ||
      user_dict_->readonly() ||
      !ctx ||
      !ctx->HasMenu())
    return;
  auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(
          ctx->GetSelectedCandidate()));
  if (Language::intelligible(phrase, this)) {
    const DictEntry& entry(phrase->entry());
    LOG(INFO) << "deleting entry: '" << entry.text << "'.";
    user_dict_->UpdateEntry(entry, -1);  // mark as deleted in user dict
    ctx->RefreshNonConfirmedComposition();
  }
}

void Memory::OnUnhandledKey(Context* ctx, const KeyEvent& key) {
  if (!user_dict_ || user_dict_->readonly())
    return;
  if ((key.modifier() & ~kShiftMask) == 0) {
    if (key.keycode() == XK_BackSpace && DiscardSession()) {
      return;  // forget about last commit
    }
    FinishSession();
  }
}

}  // namespace rime
