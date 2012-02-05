// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/dict/dictionary.h>
#include <rime/algo/syllabifier.h>

namespace rime {

namespace dictionary {

const std::string RawCode::ToString() const {
  return boost::join(*this, " ");
}

void RawCode::FromString(const std::string &code) {
  boost::split(*dynamic_cast<std::vector<std::string> *>(this),
               code,
               boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
}

bool compare_chunk_by_head_element(const Chunk &a, const Chunk &b) {
  if (!a.entries || a.cursor >= a.size) return false;
  if (!b.entries || b.cursor >= b.size) return true;
  return a.entries[a.cursor].weight > b.entries[b.cursor].weight;  // by weight desc
}

size_t match_extra_code(const table::Code *extra_code, size_t depth,
                        const SyllableGraph &syll_graph, size_t current_pos) {
  if (!extra_code || depth >= extra_code->size)
    return current_pos;  // success
  if (current_pos >= syll_graph.interpreted_length)
    return 0;  // failure (possibly success for completion in the future)
  EdgeMap::const_iterator edges = syll_graph.edges.find(current_pos);
  if (edges == syll_graph.edges.end()) {
    return 0;
  }
  table::SyllableId current_syll_id = extra_code->at[depth];
  size_t best_match = 0;
  BOOST_FOREACH(const EndVertexMap::value_type &edge, edges->second) {
    size_t end_vertex_pos = edge.first;
    const SpellingMap &spellings(edge.second);
    if (spellings.find(current_syll_id) == spellings.end())
      continue;
    size_t match_end_pos = match_extra_code(extra_code, depth + 1,
                                            syll_graph, end_vertex_pos);
    if (!match_end_pos) continue;
    if (match_end_pos > best_match) best_match = match_end_pos;
  }
  return best_match;
}

}  // namespace dictionary

DictEntryIterator::DictEntryIterator()
    : Base(), entry_(), entry_count_(0) {
}

DictEntryIterator::DictEntryIterator(const DictEntryIterator &other)
    : Base(other), entry_(other.entry_), entry_count_(other.entry_count_) {
}

DictEntryIterator& DictEntryIterator::operator= (DictEntryIterator &other) {
  EZLOGGERPRINT("swapping iterator contents.");
  swap(other);
  entry_ = other.entry_;
  entry_count_ = other.entry_count_;
  return *this;
}

bool DictEntryIterator::exhausted() const {
  return empty();
}

void DictEntryIterator::AddChunk(const dictionary::Chunk &chunk) {
  push_back(chunk);
  entry_count_ += chunk.size;
}

void DictEntryIterator::Sort() {
  sort(dictionary::compare_chunk_by_head_element);
}

shared_ptr<DictEntry> DictEntryIterator::Peek() {
  if (empty()) {
    EZLOGGERPRINT("Oops!");
    return shared_ptr<DictEntry>();
  }
  if (!entry_) {
    const dictionary::Chunk &chunk(front());
    entry_.reset(new DictEntry);
    const table::Entry &e(chunk.entries[chunk.cursor]);
    EZDBGONLYLOGGERPRINT("Creating temporary dict entry '%s'.", e.text.c_str());
    entry_->code = chunk.code;
    entry_->text = e.text.c_str();
    if (!chunk.remaining_code.empty()) {
      entry_->comment = "~" + chunk.remaining_code;
    }
    const double kS = 100000.0;
    entry_->weight = (e.weight + 1) / kS;
  }
  return entry_;
}

bool DictEntryIterator::Next() {
  if (empty()) {
    return false;
  }
  dictionary::Chunk &chunk(front());
  if (++chunk.cursor >= chunk.size) {
    pop_front();
  }
  else {
    // reorder chunks since front() has got a new head element
    Sort();
  }
  // unload retired entry
  entry_.reset();
  return !empty();
}

bool DictEntryIterator::Skip(size_t num_entries) {
  while (num_entries > 0) {
    if (empty()) return false;
    dictionary::Chunk &chunk(front());
    if (chunk.cursor + num_entries < chunk.size) {
      chunk.cursor += num_entries;
      return true;
    }
    num_entries -= (chunk.size - chunk.cursor);
    pop_front();
  }
  return true;
}

// Dictionary members

Dictionary::Dictionary(const std::string &name,
                       const shared_ptr<Table> &table, const shared_ptr<Prism> &prism)
    : name_(name), table_(table), prism_(prism) {
}

Dictionary::~Dictionary() {
  // should not close shared table and prism objects
}

shared_ptr<DictEntryCollector> Dictionary::Lookup(const SyllableGraph &syllable_graph,
                                                  size_t start_pos) {
  if (!loaded())
    return shared_ptr<DictEntryCollector>();
  TableQueryResult result;
  if (!table_->Query(syllable_graph, start_pos, &result)) {
    return shared_ptr<DictEntryCollector>();
  }
  shared_ptr<DictEntryCollector> collector(new DictEntryCollector);
  // copy result
  BOOST_FOREACH(TableQueryResult::value_type &v, result) {
    size_t end_pos = v.first;
    BOOST_FOREACH(TableAccessor &a, v.second) {
      if (a.extra_code()) {
        do {
          size_t actual_end_pos = dictionary::match_extra_code(a.extra_code(), 0,
                                                               syllable_graph, end_pos);
          if (actual_end_pos == 0) continue;
          (*collector)[actual_end_pos].AddChunk(dictionary::Chunk(a.code(), a.entry()));
        }
        while (a.Next());
      }
      else {
        (*collector)[end_pos].AddChunk(dictionary::Chunk(a));
      }
    }
  }
  // sort each group of equal code length
  BOOST_FOREACH(DictEntryCollector::value_type &v, *collector) {
    v.second.Sort();
  }
  return collector;
}

size_t Dictionary::LookupWords(DictEntryIterator *result,
                               const std::string &str_code,
                               bool predictive,
                               size_t expand_search_limit) {
  EZDBGONLYLOGGERVAR(str_code);
  if (!loaded())
    return 0;
  std::vector<Prism::Match> keys;
  if (predictive) {
    prism_->ExpandSearch(str_code, &keys, expand_search_limit);
  }
  else {
    Prism::Match match = {0, 0};
    if (prism_->GetValue(str_code, &match.value)) {
      keys.push_back(match);
    }
  }
  EZDBGONLYLOGGERPRINT("found %u matching keys thru the prism.", keys.size());
  size_t code_length(str_code.length());
  BOOST_FOREACH(Prism::Match &match, keys) {
    SpellingAccessor accessor(prism_->QuerySpelling(match.value));
    while (!accessor.exhausted()) {
      int syllable_id = accessor.syllable_id();
      SpellingType type = accessor.properties().type;
      accessor.Next();
      if (type > kNormalSpelling) continue;
      std::string remaining_code;
      if (match.length > code_length) {
        const char *syllable = table_->GetSyllableById(syllable_id);
        size_t syllable_code_length = syllable ? strlen(syllable) : 0;
        if (syllable_code_length > code_length)
          remaining_code = syllable + code_length;
      }
      const TableAccessor a(table_->QueryWords(syllable_id));
      if (!a.exhausted()) {
        EZDBGONLYLOGGERVAR(remaining_code);
        result->AddChunk(dictionary::Chunk(a, remaining_code));
      }
    }
  }
  return keys.size();
}

bool Dictionary::Decode(const Code &code, dictionary::RawCode *result) {
  if (!result || !table_)
    return false;
  result->clear();
  BOOST_FOREACH(int c, code) {
    const char *s = table_->GetSyllableById(c);
    if (!s)
      return false;
    result->push_back(s);
  }
  return true;
}

bool Dictionary::Exists() const {
  return boost::filesystem::exists(prism_->file_name()) &&
         boost::filesystem::exists(table_->file_name());
}

bool Dictionary::Remove() {
  if (loaded()) return false;
  prism_->Remove();
  table_->Remove();
  return true;
}

bool Dictionary::Load() {
  EZLOGGERFUNCTRACKER;
  if (!table_ || !table_->IsOpen() && !table_->Load()) {
    EZLOGGERPRINT("Error loading table for dictionary '%s'.", name_.c_str());
    return false;
  }
  if (!prism_ || !prism_->IsOpen() && !prism_->Load()) {
    EZLOGGERPRINT("Error loading prism for dictionary '%s'.", name_.c_str());
    return false;
  }
  return true;
}

bool Dictionary::loaded() const {
  return table_ && table_->IsOpen() && prism_ && prism_->IsOpen();
}

// DictionaryComponent members

DictionaryComponent::DictionaryComponent() {
}

Dictionary* DictionaryComponent::Create(Schema *schema) {
  if (!schema) return NULL;
  return CreateDictionaryFromConfig(schema->config(), "translator");
}

Dictionary* DictionaryComponent::CreateDictionaryFromConfig(
    Config *config, const std::string &customer) {
  std::string dict_name;
  if (!config->GetString(customer + "/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified for %s.", customer.c_str());
    return NULL;
  }
  std::string prism_name;
  if (!config->GetString(customer + "/prism", &prism_name)) {
    // usually same with dictionary name; different for alternative spelling
    prism_name = dict_name;
  }
  return CreateDictionaryWithName(dict_name, prism_name);
}

Dictionary* DictionaryComponent::CreateDictionaryWithName(
    const std::string &dict_name, const std::string &prism_name) {
  // obtain prism and table objects
  boost::filesystem::path path(Service::instance().deployer().user_data_dir);
  shared_ptr<Table> table(table_map_[dict_name].lock());
  if (!table) {
    table.reset(new Table((path / dict_name).string() + ".table.bin"));
    table_map_[dict_name] = table;
  }
  shared_ptr<Prism> prism(prism_map_[prism_name].lock());
  if (!prism) {
    prism.reset(new Prism((path / prism_name).string() + ".prism.bin"));
    prism_map_[prism_name] = prism;
  }
  return new Dictionary(dict_name, table, prism);
}

}  // namespace rime
