//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/candidate.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/gear/reverse_lookup_translator.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/table_translator.h>


//static const char* quote_left = "\xef\xbc\x88";
//static const char* quote_right = "\xef\xbc\x89";
//static const char* separator = "\xef\xbc\x8c";

namespace rime {

class ReverseLookupTranslation : public TableTranslation {
 public:
  ReverseLookupTranslation(ReverseLookupDictionary* dict,
                           TranslatorOptions* options,
                           const string& input,
                           size_t start, size_t end,
                           const string& preedit,
                           DictEntryIterator&& iter,
                           bool quality)
      : TableTranslation(
            options, NULL, input, start, end, preedit, std::move(iter)),
        dict_(dict), options_(options), quality_(quality) {
  }
  virtual an<Candidate> Peek();
  virtual int Compare(an<Translation> other,
                      const CandidateList& candidates);
 protected:
  ReverseLookupDictionary* dict_;
  TranslatorOptions* options_;
  bool quality_;
};

an<Candidate> ReverseLookupTranslation::Peek() {
  if (exhausted())
    return nullptr;
  const auto& entry(iter_.Peek());
  string tips;
  if (dict_) {
    dict_->ReverseLookup(entry->text, &tips);
    if (options_) {
      options_->comment_formatter().Apply(&tips);
    }
    //if (!tips.empty()) {
    //  boost::algorithm::replace_all(tips, " ", separator);
    //}
  }
  an<Candidate> cand = New<SimpleCandidate>(
      "reverse_lookup",
      start_,
      end_,
      entry->text,
      !tips.empty() ? tips : entry->comment,
      preedit_);
  return cand;
}

int ReverseLookupTranslation::Compare(an<Translation> other,
                                      const CandidateList& candidates) {
  if (!other || other->exhausted())
    return -1;
  if (exhausted())
    return 1;
  auto theirs = other->Peek();
  if (!theirs)
    return -1;
  if (quality_ && theirs->type() == "completion")
    return -1;
  if (theirs->type() == "sentence")
    return -1;
  return 1;
}

ReverseLookupTranslator::ReverseLookupTranslator(const Ticket& ticket)
    : Translator(ticket), tag_("reverse_lookup") {
  if (ticket.name_space == "translator") {
    name_space_ = "reverse_lookup";
  }
  if (!ticket.schema)
    return;
  Config* config = ticket.schema->config();
  config->GetString(name_space_ + "/tag", &tag_);
}

void ReverseLookupTranslator::Initialize() {
  initialized_ = true;  // no retry
  if (!engine_)
    return;
  Ticket ticket(engine_, name_space_);
  options_.reset(new TranslatorOptions(ticket));
  Config* config = engine_->schema()->config();
  if (!config)
    return;
  config->GetString(name_space_ + "/prefix", &prefix_);
  config->GetString(name_space_ + "/suffix", &suffix_);
  config->GetString(name_space_ + "/tips", &tips_);
  {
    bool enabled = false;
    if (!config->GetBool(name_space_ + "/enable_completion", &enabled))
      options_->set_enable_completion(false);  // overridden default
  }

  if (auto component = Dictionary::Require("dictionary")) {
    dict_.reset(component->Create(ticket));
  }
  if (dict_) {
    dict_->Load();
  }
  else {
    return;
  }
  auto rev_component =
      ReverseLookupDictionary::Require("reverse_lookup_dictionary");
  if (!rev_component)
    return;
  // lookup target defaults to "translator/dictionary"
  string rev_target("translator");
  config->GetString(name_space_ + "/target", &rev_target);
  Ticket rev_ticket(engine_, rev_target);
  rev_dict_.reset(rev_component->Create(rev_ticket));
  if (rev_dict_) {
    rev_dict_->Load();
  }
}

an<Translation> ReverseLookupTranslator::Query(const string& input,
                                                       const Segment& segment) {
  if (!segment.HasTag(tag_))
    return nullptr;
  if (!initialized_)
    Initialize();  // load reverse dict at first use
  if (!dict_ || !dict_->loaded())
    return nullptr;
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  const string& preedit(input);

  size_t start = 0;
  if (!prefix_.empty() && boost::starts_with(input, prefix_)) {
    start = prefix_.length();
  }
  string code = input.substr(start);
  if (!suffix_.empty() && boost::ends_with(code, suffix_)) {
    code.resize(code.length() - suffix_.length());
  }

  if (start > 0) {
    // usually translators do not modify the segment directly;
    // prompt text is best set by a processor or a segmentor.
    const_cast<Segment*>(&segment)->prompt = tips_;
  }

  DictEntryIterator iter;
  bool quality = false;
  if (start < input.length()) {
    if (options_ && options_->enable_completion()) {
      dict_->LookupWords(&iter, code, true, 100);
      quality = !iter.exhausted() &&
                (iter.Peek()->remaining_code_length == 0);
    }
    else {
      // 2012-04-08 gongchen: fetch multi-syllable words from rev-lookup table
      SyllableGraph graph;
      Syllabifier syllabifier("", true, options_->strict_spelling());
      size_t consumed = syllabifier.BuildSyllableGraph(code,
                                                       *dict_->prism(),
                                                       &graph);
      if (consumed == code.length()) {
        auto collector = dict_->Lookup(graph, 0);
        if (collector && !collector->empty() &&
            collector->rbegin()->first == consumed) {
          iter = std::move(collector->rbegin()->second);
          quality = !graph.vertices.empty() &&
              (graph.vertices.rbegin()->second == kNormalSpelling);
        }
      }
    }
  }
  if (!iter.exhausted()) {
    return Cached<ReverseLookupTranslation>(rev_dict_.get(),
                                            options_.get(),
                                            code,
                                            segment.start,
                                            segment.end,
                                            preedit,
                                            std::move(iter),
                                            quality);
  }
  return nullptr;
}

}  // namespace rime
