// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/impl/table_translator.h>

namespace rime {

class LazyTableTranslation : public TableTranslation {
 public:
  static const size_t kInitialSearchLimit = 10;
  static const size_t kExpandingFactor = 10;
  
  LazyTableTranslation(const std::string &input,
                       size_t start, size_t end,
                       const std::string &preedit,
                       Projection *comment_formatter,
                       Dictionary *dict);
  virtual bool Next();
  
 private:
  Dictionary *dict_;
  size_t limit_;
};

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
  shared_ptr<Candidate> cand(new SimpleCandidate(
      "zh",
      start_,
      end_,
      e->text,
      comment,
      preedit_));
  return cand;
}

LazyTableTranslation::LazyTableTranslation(const std::string &input,
                                           size_t start, size_t end,
                                           const std::string &preedit,
                                           Projection *comment_formatter,
                                           Dictionary *dict)
    : TableTranslation(input, start, end, preedit, comment_formatter),
      dict_(dict), limit_(kInitialSearchLimit) {
  dict->LookupWords(&iter_, input, true, kInitialSearchLimit);
  set_exhausted(iter_.exhausted());
}

bool LazyTableTranslation::Next() {
  if (exhausted())
    return false;
  iter_.Next();
  if (limit_ > 0 && iter_.exhausted()) {
    limit_ *= kExpandingFactor;
    size_t previous_entry_count = iter_.entry_count();
    EZLOGGERPRINT("fetching more entries: limit = %d, count = %d.",
                  limit_, previous_entry_count);
    DictEntryIterator more;
    if (dict_->LookupWords(&more, input_, true, limit_) < limit_) {
      EZLOGGERPRINT("all entries obtained.");
      limit_ = 0;  // no more try
    }
    if (more.entry_count() > previous_entry_count) {
      more.Skip(previous_entry_count);
      iter_ = more;
    }
  }
  set_exhausted(iter_.exhausted());
  return true;
}

TableTranslator::TableTranslator(Engine *engine)
    : Translator(engine),
      enable_completion_(true) {
  if (!engine) return;
  
  Config *config = engine->schema()->config();
  if (config) {
    config->GetBool("translator/enable_completion", &enable_completion_);
    preedit_formatter_.Load(config->GetList("translator/preedit_format"));
    comment_formatter_.Load(config->GetList("translator/comment_format"));
  }

  Dictionary::Component *component = Dictionary::Require("dictionary");
  if (!component) return;
  dict_.reset(component->Create(engine->schema()));
  dict_->Load();
}

TableTranslator::~TableTranslator() {
}

Translation* TableTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("abc"))
    return NULL;
  EZDBGONLYLOGGERPRINT("input = '%s', [%d, %d)",
                       input.c_str(), segment.start, segment.end);

  std::string preedit(input);
  preedit_formatter_.Apply(&preedit);
  
  Translation *translation = NULL;
  if (enable_completion_) {
    translation = new LazyTableTranslation(input,
                                           segment.start,
                                           segment.start + input.length(),
                                           preedit,
                                           &comment_formatter_,
                                           dict_.get());
  }
  else {
    DictEntryIterator iter;
    dict_->LookupWords(&iter, input, false);
    if (!iter.exhausted())
      translation = new TableTranslation(iter,
                                         input,
                                         segment.start,
                                         segment.start + input.length(),
                                         preedit,
                                         &comment_formatter_);
  }
  return translation;
}

}  // namespace rime
