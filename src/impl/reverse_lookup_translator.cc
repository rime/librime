// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/impl/table_translator.h>
#include <rime/impl/reverse_lookup_translator.h>

static const char *quote_left = "\xef\xbc\x88";
static const char *quote_right = "\xef\xbc\x89";
static const char *separator = "\xef\xbc\x8c";

namespace rime {

class ReverseLookupTranslation : public TableTranslation {
 public:
  ReverseLookupTranslation(const DictEntryIterator &iter,
                           const std::string &input,
                           size_t start, size_t end,
                           const std::string &preedit,
                           ReverseLookupDictionary *dict)
      : TableTranslation(iter, input, start, end, preedit), dict_(dict) {}
  virtual shared_ptr<Candidate> Peek();
 protected:
  ReverseLookupDictionary *dict_;
};

shared_ptr<Candidate> ReverseLookupTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  std::string tips;
  if (dict_) {
    dict_->ReverseLookup(e->text, &tips);
    if (!tips.empty()) {
      boost::algorithm::replace_all(tips, " ", separator);
    }
  }
  shared_ptr<Candidate> cand(new SimpleCandidate(
      "reverse_lookup",
      start_,
      end_,
      e->text,
      !tips.empty() ? (quote_left + tips + quote_right) : e->comment,
      preedit_));
  return cand;
}

ReverseLookupTranslator::ReverseLookupTranslator(Engine *engine)
    : Translator(engine) {
  if (!engine) return;
  Config *config = engine->schema()->config();
  if (!config) return;
  config->GetString("reverse_lookup/prefix", &prefix_);
  config->GetString("reverse_lookup/tips", &tips_);
  formatter_.Load(config->GetList("reverse_lookup/preedit_format"));
  
  DictionaryComponent *component = dynamic_cast<DictionaryComponent*>(
      Dictionary::Require("dictionary"));
  if (!component) return;
  dict_.reset(component->CreateDictionaryFromConfig(config, "reverse_lookup"));
  if (dict_) 
    dict_->Load();
  else
    return;
  ReverseLookupDictionary::Component *rev_component =
      ReverseLookupDictionary::Require("reverse_lookup_dictionary");
  if (!rev_component) return;
  rev_dict_.reset(rev_component->Create(engine->schema()));
  if (rev_dict_)
    rev_dict_->Load();
}

Translation* ReverseLookupTranslator::Query(const std::string &input,
                                            const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("reverse_lookup"))
    return NULL;
  EZDBGONLYLOGGERPRINT("input = '%s', [%d, %d)",
                       input.c_str(), segment.start, segment.end);

  size_t start = 0;
  if (boost::starts_with(input, prefix_))
    start = prefix_.length();
  std::string code(input.substr(start));
  
  Translation *translation = NULL;
  DictEntryIterator iter;
  if (start < input.length())
    dict_->LookupWords(&iter, code, false);
  if (!iter.exhausted()) {
    std::string preedit(input);
    formatter_.Apply(&preedit);
    translation = new ReverseLookupTranslation(iter,
                                               code,
                                               segment.start,
                                               segment.end,
                                               preedit,
                                               rev_dict_.get());
  }
  else {
    shared_ptr<Candidate> cand(new SimpleCandidate("raw",
                                                   segment.start, segment.end,
                                                   input, tips_));
    return new UniqueTranslation(cand);
  }
  return translation;
}

}  // namespace rime
