//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/candidate.h>
#include <rime/context.h>
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

// static const char* quote_left = "\xef\xbc\x88";
// static const char* quote_right = "\xef\xbc\x89";
// static const char* separator = "\xef\xbc\x8c";

namespace rime {

namespace {

void ApplyTabConstraints(SyllableGraph* graph,
                         Dictionary* dict,
                         const vector<Context::TabConstraint>& constraints,
                         size_t segment_start,
                         size_t segment_end,
                         size_t code_start) {
  bool changed = false;
  size_t raw_pos = segment_start;
  size_t shadow_pos = 0;
  for (const auto& constraint : constraints) {
    if (constraint.position < raw_pos)
      continue;
    if (constraint.position >= segment_end)
      break;

    shadow_pos += constraint.position - raw_pos;
    raw_pos = (std::min)(constraint.position + constraint.span, segment_end);

    if (shadow_pos < code_start) {
      shadow_pos += constraint.label.length();
      continue;
    }
    const size_t graph_pos = shadow_pos - code_start;
    const size_t span = constraint.label.length();
    shadow_pos += span;
    if (graph_pos >= graph->input_length || span == 0)
      continue;

    auto edge_it = graph->edges.find(graph_pos);
    if (edge_it == graph->edges.end())
      continue;

    const string label = StripTones(constraint.label);
    auto& end_map = edge_it->second;
    for (auto end_it = end_map.begin(); end_it != end_map.end();) {
      if (end_it->first != graph_pos + span) {
        end_it = end_map.erase(end_it);
        changed = true;
        continue;
      }
      auto& spelling_map = end_it->second;
      for (auto spelling_it = spelling_map.begin();
           spelling_it != spelling_map.end();) {
        Code code;
        code.push_back(spelling_it->first);
        vector<string> decoded;
        if (dict->Decode(code, &decoded) && !decoded.empty() &&
            StripTones(decoded[0]) == label) {
          ++spelling_it;
        } else {
          spelling_it = spelling_map.erase(spelling_it);
          changed = true;
        }
      }
      if (spelling_map.empty()) {
        end_it = end_map.erase(end_it);
        changed = true;
      } else {
        ++end_it;
      }
    }
  }

  if (!changed)
    return;
  graph->indices.clear();
  for (const auto& [start, end_map] : graph->edges) {
    auto& index = graph->indices[start];
    for (const auto& [end, spelling_map] : end_map) {
      for (const auto& [syllable_id, properties] : spelling_map) {
        index[syllable_id].push_back(&properties);
      }
    }
  }
}

}  // namespace

class ReverseLookupTranslation : public TableTranslation {
 public:
  ReverseLookupTranslation(ReverseLookupDictionary* dict,
                           TranslatorOptions* options,
                           const string& input,
                           size_t start,
                           size_t end,
                           const string& preedit,
                           DictEntryIterator&& iter,
                           bool quality)
      : TableTranslation(options,
                         NULL,
                         nullptr,
                         input,
                         start,
                         end,
                         preedit,
                         std::move(iter)),
        dict_(dict),
        options_(options),
        quality_(quality) {}
  virtual an<Candidate> Peek();
  virtual int Compare(an<Translation> other, const CandidateList& candidates);

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
    // if (!tips.empty()) {
    //   boost::algorithm::replace_all(tips, " ", separator);
    // }
  }
  an<Candidate> cand =
      New<SimpleCandidate>("reverse_lookup", start_, end_, entry->text,
                           !tips.empty() ? tips : entry->comment, preedit_);
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
  } else {
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
  const string shadow_input =
      engine_->context()->shadow_input(segment.start, segment.end);
  DLOG(INFO) << "input = '" << shadow_input << "', [" << segment.start << ", "
             << segment.end << ")";

  const string& preedit(input);

  size_t start = 0;
  if (!prefix_.empty() && boost::starts_with(shadow_input, prefix_)) {
    start = prefix_.length();
  }
  string code = shadow_input.substr(start);
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
  if (start < shadow_input.length()) {
    if (options_ && options_->enable_completion()) {
      dict_->LookupWords(&iter, code, true, 100, nullptr);
      quality = !iter.exhausted() && (iter.Peek()->remaining_code_length == 0);
    } else {
      // 2012-04-08 gongchen: fetch multi-syllable words from rev-lookup table
      SyllableGraph graph;
      Syllabifier syllabifier("", true, options_->strict_spelling());
      size_t consumed =
          syllabifier.BuildSyllableGraph(code, *dict_->prism(), &graph);
      ApplyTabConstraints(&graph, dict_.get(),
                          engine_->context()->tab_constraints(), segment.start,
                          segment.end, start);
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
    return Cached<ReverseLookupTranslation>(rev_dict_.get(), options_.get(),
                                            code, segment.start, segment.end,
                                            preedit, std::move(iter), quality);
  }
  return nullptr;
}

void ReverseLookupTranslator::CollectReverseLookupTabs(
    size_t start_pos,
    vector<InputTabEntry>* tabs) const {
  if (!initialized_)
    const_cast<ReverseLookupTranslator*>(this)->Initialize();
  if (!dict_ || !dict_->loaded() || !engine_ || !engine_->context())
    return;

  const string& input = engine_->context()->input();
  if (start_pos >= input.length()) {
    return;
  }

  // Find the segment containing start_pos
  const auto& comp = engine_->context()->composition();
  const Segment* seg = nullptr;
  for (const auto& s : comp) {
    if (start_pos >= s.start && start_pos < s.end) {
      seg = &s;
      break;
    }
  }
  if (!seg) {
    return;
  }

  // Use shadow_input to get display text (pinyin for reverse lookup)
  string shadow = engine_->context()->shadow_input(seg->start, seg->end);
  size_t rel_pos = start_pos - seg->start;

  if (rel_pos >= shadow.length()) {
    return;
  }

  string code = shadow.substr(rel_pos);
  size_t prefix_len = 0;
  if (!prefix_.empty()) {
    if (!boost::starts_with(shadow, prefix_)) {
      return;
    }
    if (rel_pos < prefix_.length()) {
      code = shadow.substr(prefix_.length());
      prefix_len = prefix_.length() - rel_pos;
    } else {
      prefix_len = 0;
    }
  }
  if (!suffix_.empty() && boost::ends_with(code, suffix_)) {
    code.resize(code.length() - suffix_.length());
  }
  if (code.empty()) {
    return;
  }

  // Build SyllableGraph from the code
  SyllableGraph graph;
  Syllabifier syllabifier("", true,
                          options_ ? options_->strict_spelling() : false);
  size_t consumed =
      syllabifier.BuildSyllableGraph(code, *dict_->prism(), &graph);
  if (consumed == 0) {
    return;
  }

  // Apply tab constraints to filter the graph (same as Query)
  ApplyTabConstraints(&graph, dict_.get(),
                      engine_->context()->tab_constraints(), seg->start,
                      seg->end, rel_pos + prefix_len);

  // Collect edges from position 0
  auto it = graph.edges.find(0);
  if (it == graph.edges.end()) {
    return;
  }
  set<string> seen;
  for (const auto& [end_pos, syllable_map] : it->second) {
    for (const auto& [syllable_id, props] : syllable_map) {
      Code single_code;
      single_code.push_back(syllable_id);
      vector<string> decoded;
      if (dict_->Decode(single_code, &decoded) && !decoded.empty()) {
        const string& label = decoded[0];
        if (!label.empty() && seen.insert(label).second) {
          tabs->emplace_back(InputTabEntry{label, prefix_len + end_pos,
                                           InputTabEntry::kReverseLookup});
        }
      }
    }
  }
}

}  // namespace rime
