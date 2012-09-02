// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <utf8.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/composition.h>
#include <rime/engine.h>
#include <rime/dict/user_dictionary.h>
#include <rime/impl/translator_commons.h>

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

// TableTranslation

TableTranslation::TableTranslation(const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(true);
}

TableTranslation::TableTranslation(const DictEntryIterator& iter,
                                   const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : iter_(iter), input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(iter_.exhausted());
}

bool TableTranslation::Next() {
  if (exhausted())
    return false;
  iter_.Next();
  set_exhausted(iter_.exhausted());
  return true;
}

shared_ptr<Candidate> TableTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  std::string comment(e->comment);
  if (comment_formatter_) {
    comment_formatter_->Apply(&comment);
  }
  return boost::make_shared<SimpleCandidate>(
      e->remaining_code_length == 0 ? "zh" : "completion",
      start_,
      end_,
      e->text,
      comment,
      preedit_);
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

void Memory::OnDeleteEntry(Context *ctx) {
  if (!user_dict_ ||
      !ctx ||
      ctx->composition()->empty())
    return;
  Segment &seg(ctx->composition()->back());
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (!cand)
    return;
  shared_ptr<UniquifiedCandidate> uniquified = As<UniquifiedCandidate>(cand);
  if (uniquified) cand = uniquified->items().front();
  shared_ptr<ShadowCandidate> shadow = As<ShadowCandidate>(cand);
  if (shadow) cand = shadow->item();
  shared_ptr<ZhCandidate> zh = As<ZhCandidate>(cand);
  if (zh) {
    const DictEntry& entry(zh->entry());
    LOG(INFO) << "deleting entry: '" << entry.text << "'.";
    user_dict_->UpdateEntry(entry, -1);  // mark as deleted in user dict
    ctx->RefreshNonConfirmedComposition();
  }
}

}  // namespace rime
