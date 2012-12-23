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
#include <rime/algo/syllabifier.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/gear/reverse_lookup_translator.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/table_translator.h>


static const char *quote_left = "\xef\xbc\x88";
static const char *quote_right = "\xef\xbc\x89";
static const char *separator = "\xef\xbc\x8c";

namespace rime {

class ReverseLookupTranslation : public TableTranslation {
 public:
  ReverseLookupTranslation(ReverseLookupDictionary* dict,
                           TranslatorOptions* options,
                           const std::string& input,
                           size_t start, size_t end,
                           const std::string& preedit,
                           const DictEntryIterator &iter)
      : TableTranslation(options, NULL, input, start, end, preedit, iter),
        dict_(dict), options_(options) {
  }
  virtual shared_ptr<Candidate> Peek();
  virtual int Compare(shared_ptr<Translation> other,
                      const CandidateList &candidates);
 protected:
  ReverseLookupDictionary* dict_;
  TranslatorOptions* options_;
};

shared_ptr<Candidate> ReverseLookupTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  std::string tips;
  if (dict_) {
    dict_->ReverseLookup(e->text, &tips);
    if (options_) {
      options_->comment_formatter().Apply(&tips);
    }
    if (!tips.empty()) {
      boost::algorithm::replace_all(tips, " ", separator);
    }
  }
  shared_ptr<Candidate> cand = boost::make_shared<SimpleCandidate>(
      "reverse_lookup",
      start_,
      end_,
      e->text,
      !tips.empty() ? (quote_left + tips + quote_right) : e->comment,
      preedit_);
  return cand;
}

int ReverseLookupTranslation::Compare(shared_ptr<Translation> other,
                                      const CandidateList &candidates) {
  if (!other || other->exhausted()) return -1;
  if (exhausted()) return 1;
  shared_ptr<const Candidate> theirs = other->Peek();
  if (!theirs)
    return -1;
  if (theirs->type() == "completion" || theirs->type() == "sentence")
    return -1;
  return 1;
}

ReverseLookupTranslator::ReverseLookupTranslator(Engine *engine)
    : Translator(engine), initialized_(false) {
}

void ReverseLookupTranslator::Initialize() {
  initialized_ = true;  // no retry
  if (!engine_) return;
  options_.reset(new TranslatorOptions(engine_, "reverse_lookup"));
  Config *config = engine_->schema()->config();
  if (!config) return;
  config->GetString("reverse_lookup/prefix", &prefix_);
  config->GetString("reverse_lookup/suffix", &suffix_);
  config->GetString("reverse_lookup/tips", &tips_);
  
  DictionaryComponent *component =
      dynamic_cast<DictionaryComponent*>(Dictionary::Require("dictionary"));
  if (!component) return;
  dict_.reset(component->CreateDictionaryFromConfig(config, "reverse_lookup"));
  if (dict_) 
    dict_->Load();
  else
    return;
  ReverseLookupDictionary::Component *rev_component =
      ReverseLookupDictionary::Require("reverse_lookup_dictionary");
  if (!rev_component) return;
  rev_dict_.reset(rev_component->Create(engine_->schema()));
  if (rev_dict_)
    rev_dict_->Load();
}

shared_ptr<Translation> ReverseLookupTranslator::Query(const std::string &input,
                                                       const Segment &segment,
                                                       std::string* prompt) {
  if (!segment.HasTag("reverse_lookup"))
    return shared_ptr<Translation>();
  if (!initialized_) Initialize();  // load reverse dict at first use
  if (!dict_ || !dict_->loaded())
    return shared_ptr<Translation>();
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  const std::string& preedit(input);
  
  size_t start = 0;
  if (!prefix_.empty() && boost::starts_with(input, prefix_))
    start = prefix_.length();
  std::string code(input.substr(start));
  if (!suffix_.empty() && boost::ends_with(code, suffix_))
    code.resize(code.length() - suffix_.length());
  
  if (start > 0 && prompt) {
    *prompt = tips_;
  }
  
  DictEntryIterator iter;
  if (start < input.length()) {
    if (options_ && options_->enable_completion()) {
      dict_->LookupWords(&iter, code, true, 100);
    }
    else {
      // 2012-04-08 gongchen: fetch multi-syllable words from rev-lookup table
      SyllableGraph graph;
      Syllabifier syllabifier("", true);
      size_t consumed = syllabifier.BuildSyllableGraph(code,
                                                       *dict_->prism(),
                                                       &graph);
      if (consumed == code.length()) {
        shared_ptr<DictEntryCollector> collector = dict_->Lookup(graph, 0);
        if (collector && !collector->empty() &&
            collector->rbegin()->first == consumed) {
          iter = collector->rbegin()->second;
        }
      }
    }
  }
  if (!iter.exhausted()) {
    return boost::make_shared<ReverseLookupTranslation>(rev_dict_.get(),
                                                        options_.get(),
                                                        code,
                                                        segment.start,
                                                        segment.end,
                                                        preedit,
                                                        iter);
  }
  return shared_ptr<Translation>();
}

}  // namespace rime
