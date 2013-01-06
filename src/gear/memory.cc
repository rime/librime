//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-01-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/composition.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/memory.h>
#include <rime/gear/translator_commons.h>

namespace rime {

Memory::Memory(Engine* engine) {
  if (!engine) return;

  Dictionary::Component *dictionary = Dictionary::Require("dictionary");
  if (dictionary) {
    dict_.reset(dictionary->Create(engine->schema()));
    if (dict_)
      dict_->Load();
  }
  
  UserDictionary::Component *user_dictionary =
      UserDictionary::Require("user_dictionary");
  if (user_dictionary) {
    user_dict_.reset(user_dictionary->Create(engine->schema()));
    if (user_dict_) {
      user_dict_->Load();
      if (dict_)
        user_dict_->Attach(dict_->table(), dict_->prism());
    }
  }

  commit_connection_ = engine->context()->commit_notifier().connect(
      boost::bind(&Memory::OnCommit, this, _1));
  delete_connection_ = engine->context()->delete_notifier().connect(
      boost::bind(&Memory::OnDeleteEntry, this, _1));
  unhandled_key_connection_ =
      engine->context()->unhandled_key_notifier().connect(
          boost::bind(&Memory::OnUnhandledKey, this, _1, _2));
}

Memory::~Memory() {
  commit_connection_.disconnect();
  delete_connection_.disconnect();
  unhandled_key_connection_.disconnect();
}

void Memory::OnCommit(Context* ctx) {
  if (!user_dict_) return;
  user_dict_->NewTransaction();
  DictEntry commit_entry;
  std::vector<const DictEntry*> elements;
  BOOST_FOREACH(Composition::value_type &seg, *ctx->composition()) {
    shared_ptr<Candidate> cand = seg.GetSelectedCandidate();
    shared_ptr<Phrase> phrase =
        As<Phrase>(Candidate::GetGenuineCandidate(cand));
    bool unrecognized = false;
    if (phrase && phrase->language() == language()) {
      commit_entry.text += phrase->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               phrase->code().begin(),
                               phrase->code().end());
      shared_ptr<Sentence> sentence = As<Sentence>(phrase);
      if (sentence) {
        BOOST_FOREACH(const DictEntry& e, sentence->components()) {
          elements.push_back(&e);
        }
      }
      else {
        elements.push_back(&phrase->entry());
      }
    }
    else {
      unrecognized = true;
    }
    if ((unrecognized || seg.status >= Segment::kConfirmed) &&
        !commit_entry.text.empty()) {
      DLOG(INFO) << "memorize commit entry: " << commit_entry.text;
      Memorize(commit_entry, elements);
      elements.clear();
      commit_entry.text.clear();
      commit_entry.code.clear();
    }
  }
}

void Memory::OnDeleteEntry(Context* ctx) {
  if (!user_dict_ ||
      !ctx ||
      ctx->composition()->empty())
    return;
  Segment &seg(ctx->composition()->back());
  shared_ptr<Candidate> cand = seg.GetSelectedCandidate();
  if (!cand) return;
  shared_ptr<Phrase> phrase = As<Phrase>(Candidate::GetGenuineCandidate(cand));
  if (phrase && phrase->language() == language()) {
    const DictEntry& entry(phrase->entry());
    LOG(INFO) << "deleting entry: '" << entry.text << "'.";
    user_dict_->UpdateEntry(entry, -1);  // mark as deleted in user dict
    ctx->RefreshNonConfirmedComposition();
  }
}

void Memory::OnUnhandledKey(Context* ctx, const KeyEvent& key) {
  if (!user_dict_) return;
  if ((key.modifier() & ~kShiftMask) == 0) {
    if (key.keycode() == XK_BackSpace &&
        user_dict_->RevertRecentTransaction()) {
      return;  // forget about last commit
    }
    user_dict_->CommitPendingTransaction();
  }
}

}  // namespace rime
