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
#include <rime/impl/table_translator.h>
#include <rime/impl/reverse_lookup_translator.h>

namespace rime {

class ReverseLookupTranslation : public TableTranslation {
 public:
  ReverseLookupTranslation(const DictEntryIterator &iter,
                           size_t start, size_t end)
      : TableTranslation(iter, start, end) {}

  virtual shared_ptr<Candidate> Peek();
};

shared_ptr<Candidate> ReverseLookupTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  // TODO: get comment from reverse lookup dictionary
  shared_ptr<Candidate> cand(new SimpleCandidate(
      "reverse_lookup",
      start_,
      end_,
      e->text,
      e->comment));
  return cand;
}

ReverseLookupTranslator::ReverseLookupTranslator(Engine *engine)
    : Translator(engine) {
  if (!engine) return;
  Config *config = engine->schema()->config();
  if (!config) return;
  config->GetString("reverse_lookup/prefix", &prefix_);
  config->GetString("reverse_lookup/tips", &tips_);
  DictionaryComponent *component = dynamic_cast<DictionaryComponent*>(
      Dictionary::Require("dictionary"));
  if (!component) return;
  dict_.reset(component->CreateDictionaryFromConfig(config, "reverse_lookup"));
  dict_->Load();
}

Translation* ReverseLookupTranslator::Query(const std::string &input,
                                            const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("reverse_lookup"))
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);

  size_t start = 0;
  if (boost::starts_with(input, prefix_))
    start = prefix_.length();
  
  Translation *translation = NULL;
  DictEntryIterator iter;
  if (start < input.length())
    dict_->LookupWords(&iter, input.substr(start), false);
  if (!iter.exhausted()) {
    translation = new ReverseLookupTranslation(iter,
                                               segment.start,
                                               segment.end);
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
