// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/impl/table_translator.h>
#include <rime/impl/translator_commons.h>

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
    if (dict_ && dict_->LookupWords(&more, input_, true, limit_) < limit_) {
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
    config->GetString("speller/delimiter", &delimiters_);
    config->GetBool("translator/enable_completion", &enable_completion_);
    preedit_formatter_.Load(config->GetList("translator/preedit_format"));
    comment_formatter_.Load(config->GetList("translator/comment_format"));
  }
  if (delimiters_.empty()) {
    delimiters_ = " ";
  }

  Dictionary::Component *component = Dictionary::Require("dictionary");
  if (!component) return;
  dict_.reset(component->Create(engine->schema()));
  if (dict_)
    dict_->Load();
}

TableTranslator::~TableTranslator() {
}

shared_ptr<Translation> TableTranslator::Query(const std::string &input,
                                               const Segment &segment,
                                               std::string* prompt) {
  if (!dict_ || !dict_->loaded())
    return shared_ptr<Translation>();
  if (!segment.HasTag("abc"))
    return shared_ptr<Translation>();
  EZDBGONLYLOGGERPRINT("input = '%s', [%d, %d)",
                       input.c_str(), segment.start, segment.end);

  std::string preedit(input);
  preedit_formatter_.Apply(&preedit);

  std::string code(input);
  boost::trim_right_if(code, boost::is_any_of(delimiters_));
  
  shared_ptr<Translation> translation;
  if (enable_completion_) {
    translation = boost::make_shared<LazyTableTranslation>(
        code,
        segment.start,
        segment.start + input.length(),
        preedit,
        &comment_formatter_,
        dict_.get());
  }
  else {
    DictEntryIterator iter;
    dict_->LookupWords(&iter, code, false);
    if (!iter.exhausted())
      translation = boost::make_shared<TableTranslation>(
          iter,
          code,
          segment.start,
          segment.start + input.length(),
          preedit,
          &comment_formatter_);
  }
  // TODO: insert cached phrases
  if (!translation || translation->exhausted()) {
    translation = MakeSentence(input, segment.start);
  }
  return translation;
}

class SentenceTranslation : public Translation {
 public:
  SentenceTranslation(shared_ptr<Sentence> sentence,
                      DictEntryCollector collector,
                      const std::string& input,
                      size_t start,
                      Projection* preedit_formatter)
      : input_(input), start_(start),
        preedit_formatter_(preedit_formatter) {
    sentence_.swap(sentence);
    collector_.swap(collector);
    if (sentence_) {
      sentence_->Offset(start);
      if (preedit_formatter_) {
        std::string preedit(input);
        preedit_formatter_->Apply(&preedit);
        sentence_->set_preedit(preedit);
      }
    }
    set_exhausted(!sentence_ && collector_.empty());
  }

  bool Next() {
    if (sentence_) {
      sentence_.reset();
    }
    else if (!collector_.empty()) {
      DictEntryCollector::reverse_iterator r(collector_.rbegin());
      if (!r->second.Next()) {
        collector_.erase(r->first);
      }
    }
    set_exhausted(!sentence_ && collector_.empty());
    return !exhausted();
  }
  
  shared_ptr<Candidate> Peek() {
    if (sentence_) {
      return sentence_;
    }
    if (collector_.empty()) {
      return shared_ptr<Candidate>();
    }
    DictEntryCollector::reverse_iterator r(collector_.rbegin());
    shared_ptr<DictEntry> entry(r->second.Peek());
    shared_ptr<ZhCandidate> result = boost::make_shared<ZhCandidate>(
        start_,
        start_ + r->first,
        entry);
    if (preedit_formatter_) {
      std::string preedit(input_.substr(0, r->first));
      preedit_formatter_->Apply(&preedit);
      result->set_preedit(preedit);
    }
    return result;
  }
  
 protected:
  shared_ptr<Sentence> sentence_;
  DictEntryCollector collector_;
  std::string input_;
  size_t start_;
  Projection* preedit_formatter_;
};

shared_ptr<Translation> TableTranslator::MakeSentence(const std::string& input,
                                                      size_t start) {
  DictEntryCollector collector;
  std::map<int, shared_ptr<Sentence> > sentences;
  sentences[0] = make_shared<Sentence>();
  for (size_t start_pos = 0; start_pos < input.length(); ++start_pos) {
    if (sentences.find(start_pos) == sentences.end())
      continue;
    std::vector<Prism::Match> matches;
    dict_->prism()->CommonPrefixSearch(input.substr(start_pos), &matches);
    if (matches.empty()) continue;
    BOOST_REVERSE_FOREACH(const Prism::Match &m, matches) {
      if (m.length == 0) continue;
      DictEntryIterator iter;
      dict_->LookupWords(&iter, input.substr(start_pos, m.length), false);
      if (iter.exhausted()) continue;
      size_t end_pos = start_pos + m.length;
      // consume trailing delimiters
      while (end_pos < input.length() &&
             delimiters_.find(input[end_pos]) != std::string::npos)
        ++end_pos;
      shared_ptr<Sentence> new_sentence =
          make_shared<Sentence>(*sentences[start_pos]);
      new_sentence->Extend(*iter.Peek(), end_pos);
      if (sentences.find(end_pos) == sentences.end() ||
          sentences[end_pos]->weight() <= new_sentence->weight()) {
        sentences[end_pos] = new_sentence;
      }
      if (start_pos == 0) {
        collector[end_pos] = iter;
      }
    }
  }
  if (sentences.find(input.length()) == sentences.end())
    return shared_ptr<Translation>();
  else
    return boost::make_shared<SentenceTranslation>(sentences[input.length()],
                                                   collector,
                                                   input,
                                                   start,
                                                   &preedit_formatter_);
}

}  // namespace rime
