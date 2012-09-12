// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <utf8.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/composition.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/translator_commons.h>

namespace rime {

// Patterns

bool Patterns::Load(ConfigListPtr patterns) {
  clear();
  if (!patterns) return false;
  for (ConfigList::Iterator it = patterns->begin(); it != patterns->end(); ++it) {
    ConfigValuePtr value = As<ConfigValue>(*it);
    if (!value) continue;
    push_back(boost::regex(value->str()));
  }
  return true;
}

// Sentence

void Sentence::Extend(const DictEntry& entry, size_t end_pos) {
  const double kEpsilon = 1e-200;
  const double kPenalty = 1e-8;
  entry_.code.insert(entry_.code.end(),
                     entry.code.begin(), entry.code.end());
  entry_.text.append(entry.text);
  entry_.weight *= (std::max)(entry.weight, kEpsilon) * kPenalty;
  components_.push_back(entry);
  syllable_lengths_.push_back(end_pos - end());
  set_end(end_pos);
  DLOG(INFO) << "extend sentence " << end_pos << ") "
             << entry_.text << " : " << entry_.weight;
}

void Sentence::Offset(size_t offset) {
  set_start(start() + offset);
  set_end(end() + offset);
}

// CharsetFilter

CharsetFilter::CharsetFilter(shared_ptr<Translation> translation)
    : translation_(translation) {
  LocateNextCandidate();
}

bool CharsetFilter::Next() {
  if (exhausted())
    return false;
  if (!translation_->Next()) {
    set_exhausted(true);
    return false;
  }
  return LocateNextCandidate();
}

shared_ptr<Candidate> CharsetFilter::Peek() {
  return translation_->Peek();
}

bool CharsetFilter::LocateNextCandidate() {
  while (!translation_->exhausted()) {
    shared_ptr<Candidate> cand = translation_->Peek();
    if (cand && Passed(cand->text()))
      return true;
    translation_->Next();
  }
  set_exhausted(true);
  return false;
}

bool CharsetFilter::Passed(const std::string& text) {
  const char* p = text.c_str();
  utf8::uint32_t c;
  while ((c = utf8::unchecked::next(p))) {
    if (c >= 0x3400 && c <= 0x4DBF ||    // CJK Unified Ideographs Extension A
        c >= 0x20000 && c <= 0x2A6DF ||  // CJK Unified Ideographs Extension B
        c >= 0x2A700 && c <= 0x2B73F ||  // CJK Unified Ideographs Extension C
        c >= 0x2B840 && c <= 0x2B81F)    // CJK Unified Ideographs Extension D
      return false;
  }
  return true;
}

// UniqueFilter

UniqueFilter::UniqueFilter(shared_ptr<Translation> translation)
    : translation_(translation) {
  set_exhausted(!translation_ || translation_->exhausted());
}

bool UniqueFilter::Next() {
  if (exhausted())
    return false;
  // skip duplicate candidates
  do {
    candidate_set_.insert(translation_->Peek()->text());
    translation_->Next();
  }
  while (!translation_->exhausted() &&
         AlreadyHas(translation_->Peek()->text()));
  if (translation_->exhausted()) {
    set_exhausted(true);
    return false;
  }
  return true;
}

shared_ptr<Candidate> UniqueFilter::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  return translation_->Peek();
}

bool UniqueFilter::AlreadyHas(const std::string& text) const {
  return candidate_set_.find(text) != candidate_set_.end();
}

// Memory

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
}

Memory::~Memory() {
  commit_connection_.disconnect();
  delete_connection_.disconnect();
}

void Memory::OnCommit(Context* ctx) {
  if (!user_dict_) return;
  DictEntry commit_entry;
  std::vector<const DictEntry*> elements;
  BOOST_FOREACH(Composition::value_type &seg, *ctx->composition()) {
    shared_ptr<Candidate> cand = seg.GetSelectedCandidate();
    shared_ptr<UniquifiedCandidate> uniquified = As<UniquifiedCandidate>(cand);
    if (uniquified) cand = uniquified->items().front();
    shared_ptr<ShadowCandidate> shadow = As<ShadowCandidate>(cand);
    if (shadow) cand = shadow->item();
    shared_ptr<Phrase> phrase = As<Phrase>(cand);
    shared_ptr<Sentence> sentence = As<Sentence>(cand);
    bool unrecognized = false;
    if (phrase && phrase->language() == language()) {
      commit_entry.text += phrase->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               phrase->code().begin(),
                               phrase->code().end());
      elements.push_back(&phrase->entry());
    }
    else if (sentence && sentence->language() == language()) {
      commit_entry.text += sentence->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               sentence->code().begin(),
                               sentence->code().end());
      BOOST_FOREACH(const DictEntry& e, sentence->components()) {
        elements.push_back(&e);
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
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (!cand) return;
  shared_ptr<UniquifiedCandidate> uniquified = As<UniquifiedCandidate>(cand);
  if (uniquified) cand = uniquified->items().front();
  shared_ptr<ShadowCandidate> shadow = As<ShadowCandidate>(cand);
  if (shadow) cand = shadow->item();
  shared_ptr<Phrase> phrase = As<Phrase>(cand);
  if (phrase && phrase->language() == language()) {
    const DictEntry& entry(phrase->entry());
    LOG(INFO) << "deleting entry: '" << entry.text << "'.";
    user_dict_->UpdateEntry(entry, -1);  // mark as deleted in user dict
    ctx->RefreshNonConfirmedComposition();
  }
}

// TranslatorOptions

TranslatorOptions::TranslatorOptions(Engine* engine,
                                     const std::string& prefix)
    : enable_completion_(prefix == "translator") {
  if (!engine) return;
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/delimiter", &delimiters_);
    config->GetBool(prefix + "/enable_completion", &enable_completion_);
    preedit_formatter_.Load(config->GetList(prefix + "/preedit_format"));
    comment_formatter_.Load(config->GetList(prefix + "/comment_format"));
    user_dict_disabling_patterns_.Load(
        config->GetList(prefix + "/disable_user_dict_for_patterns"));
  }
  if (delimiters_.empty()) {
    delimiters_ = " ";
  }
}

bool TranslatorOptions::IsUserDictDisabledFor(const std::string& input) const {
  if (user_dict_disabling_patterns_.empty())
    return false;
  BOOST_FOREACH(const boost::regex& pattern, user_dict_disabling_patterns_) {
    if (boost::regex_match(input, pattern))
      return true;
  }
  return false;
}


}  // namespace rime
