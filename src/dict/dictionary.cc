//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <utility>
#include <boost/filesystem.hpp>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/ticket.h>
#include <rime/dict/dictionary.h>
#include <rime/algo/syllabifier.h>

namespace rime {

namespace dictionary {

bool compare_chunk_by_head_element(const Chunk& a, const Chunk& b) {
  if (!a.entries || a.cursor >= a.size) return false;
  if (!b.entries || b.cursor >= b.size) return true;
  if (a.remaining_code.length() != b.remaining_code.length())
    return a.remaining_code.length() < b.remaining_code.length();
  return a.credibility * a.entries[a.cursor].weight >
         b.credibility * b.entries[b.cursor].weight;  // by weight desc
}

size_t match_extra_code(const table::Code* extra_code, size_t depth,
                        const SyllableGraph& syll_graph, size_t current_pos) {
  if (!extra_code || depth >= extra_code->size)
    return current_pos;  // success
  if (current_pos >= syll_graph.interpreted_length)
    return 0;  // failure (possibly success for completion in the future)
  auto index = syll_graph.indices.find(current_pos);
  if (index == syll_graph.indices.end())
    return 0;
  table::SyllableId current_syll_id = extra_code->at[depth];
  auto spellings = index->second.find(current_syll_id);
  if (spellings == index->second.end())
    return 0;
  size_t best_match = 0;
  for (const SpellingProperties* props : spellings->second) {
    size_t match_end_pos = match_extra_code(extra_code, depth + 1,
                                            syll_graph, props->end_pos);
    if (!match_end_pos) continue;
    if (match_end_pos > best_match)
      best_match = match_end_pos;
  }
  return best_match;
}

}  // namespace dictionary

DictEntryIterator::DictEntryIterator()
    : Base(), entry_(), entry_count_(0) {
}

DictEntryIterator::DictEntryIterator(const DictEntryIterator& other)
    : Base(other), entry_(other.entry_), entry_count_(other.entry_count_) {
}

DictEntryIterator& DictEntryIterator::operator= (DictEntryIterator& other) {
  DLOG(INFO) << "swapping iterator contents.";
  swap(other);
  entry_ = other.entry_;
  entry_count_ = other.entry_count_;
  return *this;
}

bool DictEntryIterator::exhausted() const {
  return empty();
}

void DictEntryIterator::AddChunk(dictionary::Chunk&& chunk) {
  push_back(std::move(chunk));
  entry_count_ += chunk.size;
}

void DictEntryIterator::Sort() {
  sort(dictionary::compare_chunk_by_head_element);
}

void DictEntryIterator::PrepareEntry() {
  if (empty()) return;
  const auto& chunk(front());
  entry_ = New<DictEntry>();
  const auto& e(chunk.entries[chunk.cursor]);
  DLOG(INFO) << "creating temporary dict entry '" << e.text.c_str() << "'.";
  entry_->code = chunk.code;
  entry_->text = e.text.c_str();
  const double kS = 1e8;
  entry_->weight = (e.weight + 1) / kS * chunk.credibility;
  if (!chunk.remaining_code.empty()) {
    entry_->comment = "~" + chunk.remaining_code;
    entry_->remaining_code_length = chunk.remaining_code.length();
  }
}

shared_ptr<DictEntry> DictEntryIterator::Peek() {
  while (!entry_ && !empty()) {
    PrepareEntry();
    if (filter_ && !filter_(entry_)) {
      Next();
    }
  }
  return entry_;
}

bool DictEntryIterator::Next() {
  entry_.reset();
  if (empty()) {
    return false;
  }
  auto& chunk(front());
  if (++chunk.cursor >= chunk.size) {
    pop_front();
  }
  else {
    // reorder chunks since front() has got a new head element
    Sort();
  }
  return !empty();
}

bool DictEntryIterator::Skip(size_t num_entries) {
  while (num_entries > 0) {
    if (empty()) return false;
    auto& chunk(front());
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

Dictionary::Dictionary(const std::string& name,
                       const shared_ptr<Table>& table,
                       const shared_ptr<Prism>& prism)
    : name_(name), table_(table), prism_(prism) {
}

Dictionary::~Dictionary() {
  // should not close shared table and prism objects
}

shared_ptr<DictEntryCollector>
Dictionary::Lookup(const SyllableGraph& syllable_graph,
                   size_t start_pos,
                   double initial_credibility) {
  if (!loaded())
    return nullptr;
  TableQueryResult result;
  if (!table_->Query(syllable_graph, start_pos, &result)) {
    return nullptr;
  }
  auto collector = New<DictEntryCollector>();
  // copy result
  for (auto& v : result) {
    size_t end_pos = v.first;
    for (TableAccessor& a : v.second) {
      double cr = initial_credibility * a.credibility();
      if (a.extra_code()) {
        do {
          size_t actual_end_pos = dictionary::match_extra_code(
              a.extra_code(), 0, syllable_graph, end_pos);
          if (actual_end_pos == 0) continue;
          (*collector)[actual_end_pos].AddChunk({a.code(), a.entry(), cr});
        }
        while (a.Next());
      }
      else {
        (*collector)[end_pos].AddChunk({a, cr});
      }
    }
  }
  // sort each group of equal code length
  for (auto& v : *collector) {
    v.second.Sort();
  }
  return collector;
}

size_t Dictionary::LookupWords(DictEntryIterator* result,
                               const std::string& str_code,
                               bool predictive,
                               size_t expand_search_limit) {
  DLOG(INFO) << "lookup: " << str_code;
  if (!loaded())
    return 0;
  std::vector<Prism::Match> keys;
  if (predictive) {
    prism_->ExpandSearch(str_code, &keys, expand_search_limit);
  }
  else {
    Prism::Match match{0, 0};
    if (prism_->GetValue(str_code, &match.value)) {
      keys.push_back(match);
    }
  }
  DLOG(INFO) << "found " << keys.size() << " matching keys thru the prism.";
  size_t code_length(str_code.length());
  for (auto& match : keys) {
    SpellingAccessor accessor(prism_->QuerySpelling(match.value));
    while (!accessor.exhausted()) {
      int syllable_id = accessor.syllable_id();
      SpellingType type = accessor.properties().type;
      accessor.Next();
      if (type > kNormalSpelling) continue;
      std::string remaining_code;
      if (match.length > code_length) {
        const char* syllable = table_->GetSyllableById(syllable_id);
        size_t syllable_code_length = syllable ? strlen(syllable) : 0;
        if (syllable_code_length > code_length)
          remaining_code = syllable + code_length;
      }
      TableAccessor a(table_->QueryWords(syllable_id));
      if (!a.exhausted()) {
        DLOG(INFO) << "remaining code: " << remaining_code;
        result->AddChunk({a, remaining_code});
      }
    }
  }
  return keys.size();
}

bool Dictionary::Decode(const Code& code, std::vector<std::string>* result) {
  if (!result || !table_)
    return false;
  result->clear();
  for (int c : code) {
    const char* s = table_->GetSyllableById(c);
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
  LOG(INFO) << "loading dictionary '" << name_ << "'.";
  if (!table_ || (!table_->IsOpen() && !table_->Load())) {
    LOG(ERROR) << "Error loading table for dictionary '" << name_ << "'.";
    return false;
  }
  if (!prism_ || (!prism_->IsOpen() && !prism_->Load())) {
    LOG(ERROR) << "Error loading prism for dictionary '" << name_ << "'.";
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

Dictionary* DictionaryComponent::Create(const Ticket& ticket) {
  if (!ticket.schema) return NULL;
  Config* config = ticket.schema->config();
  std::string dict_name;
  if (!config->GetString(ticket.name_space + "/dictionary", &dict_name)) {
    LOG(ERROR) << ticket.name_space << "/dictionary not specified in schema '"
               << ticket.schema->schema_id() << "'.";
    return NULL;
  }
  if (dict_name.empty()) {
    return NULL;  // not requiring static dictionary
  }
  std::string prism_name;
  if (!config->GetString(ticket.name_space + "/prism", &prism_name)) {
    prism_name = dict_name;
  }
  return CreateDictionaryWithName(dict_name, prism_name);
}

Dictionary*
DictionaryComponent::CreateDictionaryWithName(const std::string& dict_name,
                                              const std::string& prism_name) {
  // obtain prism and table objects
  boost::filesystem::path path(Service::instance().deployer().user_data_dir);
  auto table = table_map_[dict_name].lock();
  if (!table) {
    table = New<Table>((path / dict_name).string() + ".table.bin");
    table_map_[dict_name] = table;
  }
  auto prism = prism_map_[prism_name].lock();
  if (!prism) {
    prism = New<Prism>((path / prism_name).string() + ".prism.bin");
    prism_map_[prism_name] = prism;
  }
  return new Dictionary(dict_name, table, prism);
}

}  // namespace rime
