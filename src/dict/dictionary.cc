// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <list>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/syllablizer.h>

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

uint32_t checksum(const std::string &file_name) {
  std::ifstream fin(file_name.c_str());
  std::string file_content((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
  boost::crc_32_type crc_32;
  crc_32.process_bytes(file_content.data(), file_content.length());
  return crc_32.checksum();
}

}  // namespace dictionary

DictEntryIterator::DictEntryIterator()
    : Base(), entry_() {
}

DictEntryIterator::DictEntryIterator(const DictEntryIterator &other)
    : Base(other), entry_(other.entry_) {
}

bool DictEntryIterator::exhausted() const {
  return empty();
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
    const double kS = 10000;
    entry_->weight = e.weight / kS;
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
    // reorder chunks since front() has a new head element
    sort(dictionary::compare_chunk_by_head_element);
  }
  // unload retired entry
  entry_.reset();
  return !empty();
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
          (*collector)[actual_end_pos].push_back(dictionary::Chunk(a.code(), a.entry()));
        }
        while (a.Next());
      }
      else {
        (*collector)[end_pos].push_back(dictionary::Chunk(a));
      }
    }
  }
  // sort each group of equal code length
  BOOST_FOREACH(DictEntryCollector::value_type &v, *collector) {
    v.second.sort(dictionary::compare_chunk_by_head_element);
  }
  return collector;
}

DictEntryIterator Dictionary::LookupWords(const std::string &str_code, bool predictive) {
  EZLOGGERVAR(str_code);
  if (!loaded())
    return DictEntryIterator();
  std::vector<Prism::Match> keys;
  if (predictive) {
    const size_t kExpandSearchLimit = 512;
    prism_->ExpandSearch(str_code, &keys, kExpandSearchLimit);
  }
  else {
    Prism::Match match = {0, 0};
    if (prism_->GetValue(str_code, &match.value)) {
      keys.push_back(match);
    }
  }
  EZLOGGERPRINT("found %u matching keys thru the prism.", keys.size());
  size_t code_length(str_code.length());
  DictEntryIterator result;
  BOOST_FOREACH(Prism::Match &match, keys) {
    int syllable_id = match.value;
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
      result.push_back(dictionary::Chunk(a, remaining_code));
    }
  }
  return result;
}

bool Dictionary::Compile(const std::string &dict_file, const std::string &schema_file) {
  EZLOGGERFUNCTRACKER;
  uint32_t dict_file_checksum = dict_file.empty() ? 0 : dictionary::checksum(dict_file);
  uint32_t schema_file_checksum = schema_file.empty() ? 0 : dictionary::checksum(schema_file);
  EZLOGGERVAR(dict_file_checksum);
  EZLOGGERVAR(schema_file_checksum);
  bool rebuild_table = true;
  bool rebuild_prism = true;
  if (boost::filesystem::exists(table_->file_name()) && table_->Load()) {
    if (table_->dict_file_checksum() == dict_file_checksum) {
      rebuild_table = false;
    }
    table_->Close();
  }
  if (boost::filesystem::exists(prism_->file_name()) && prism_->Load()) {
    if (prism_->dict_file_checksum() == dict_file_checksum &&
        prism_->schema_file_checksum() == schema_file_checksum) {
      rebuild_prism = false;
    }
    prism_->Close();
  }
  if (rebuild_table && !BuildTable(dict_file, dict_file_checksum))
    return false;
  if (rebuild_prism && ! BuildPrism(schema_file, dict_file_checksum, schema_file_checksum))
    return false;
  // done!
  return true;
}

bool Dictionary::BuildTable(const std::string &dict_file, uint32_t checksum) {
  YAML::Node doc;
  {
    std::ifstream fin(dict_file.c_str());
    YAML::Parser parser(fin);
    if (!parser.GetNextDocument(doc)) {
      EZLOGGERPRINT("Error parsing yaml doc in '%s'.", dict_file.c_str());
      return false;
    }
  }
  if (doc.Type() != YAML::NodeType::Map) {
    EZLOGGERPRINT("Error: invalid yaml doc in '%s'.", dict_file.c_str());
    return false;
  }
  std::string dict_name;
  std::string dict_version;
  std::string sort_order;
  {
    const YAML::Node *name_node = doc.FindValue("name");
    const YAML::Node *version_node = doc.FindValue("version");
    const YAML::Node *sort_order_node = doc.FindValue("sort");
    if (!name_node || !version_node) {
      EZLOGGERPRINT("Error: incomplete dict info in '%s'.", dict_file.c_str());
      return false;
    }
    *name_node >> dict_name;
    *version_node >> dict_version;
    if (sort_order_node) {
      *sort_order_node >> sort_order;
    }
  }
  EZLOGGERVAR(dict_name);
  EZLOGGERVAR(dict_version);
  // read entries
  std::vector<dictionary::RawDictEntry> raw_entries;
  size_t num_entries = 0;
  Syllabary syllabary;
  {
    std::ifstream fin(dict_file.c_str());
    std::string line;
    bool in_yaml_doc = true;
    while (getline(fin, line)) {
      boost::algorithm::trim_right(line);
      // skip yaml doc
      if (in_yaml_doc) {
        if (line == "...") in_yaml_doc = false;
        continue;
      }
      // skip empty lines and comments
      if (line.empty() || line[0] == '#') continue;
      // read a dict entry
      std::string word;
      std::string str_code;
      double weight = 1.0;
      std::vector<std::string> row;
      boost::algorithm::split(row, line,
                              boost::algorithm::is_any_of("\t"));
      if (row.size() < 2 ||
          row[0].empty() || row[1].empty()) {
        EZLOGGERPRINT("Invalid entry %d.", num_entries);
        continue;
      }
      word = row[0];
      str_code = row[1];
      if (row.size() > 2) {
        try {
          weight = boost::lexical_cast<double>(row[2]);
        }
        catch (...) {
          weight = 1.0;
        }
      }

      dictionary::RawDictEntry e;
      e.raw_code.FromString(str_code);
      BOOST_FOREACH(const std::string &s, e.raw_code) {
        if (syllabary.find(s) == syllabary.end())
          syllabary.insert(s);
      }
      e.text.swap(word);
      e.weight = weight;
      raw_entries.push_back(e);
      ++num_entries;
    }
  }
  EZLOGGERVAR(num_entries);
  EZLOGGERVAR(syllabary.size());
  // build table
  {
    std::map<std::string, int> syllable_to_id;
    int syllable_id = 0;
    BOOST_FOREACH(const std::string &s, syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    Vocabulary vocabulary;
    BOOST_FOREACH(dictionary::RawDictEntry &r, raw_entries) {
      Code code;
      BOOST_FOREACH(const std::string &s, r.raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      DictEntryList *ls = vocabulary.LocateEntries(code);
      if (!ls) {
        EZLOGGERPRINT("Error locating entries in vocabulary.");
        continue;
      }
      shared_ptr<DictEntry> e(new DictEntry);
      e->code.swap(code);
      e->text.swap(r.text);
      e->weight = r.weight;
      ls->push_back(e);
    }
    if (sort_order != "original") {
      vocabulary.SortHomophones();
    }
    table_->Remove();
    if (!table_->Build(syllabary, vocabulary, num_entries, checksum) ||
        !table_->Save()) {
      return false;
    }
  }
  return true;
}

bool Dictionary::BuildPrism(const std::string &schema_file,
                            uint32_t dict_file_checksum, uint32_t schema_file_checksum) {
  // get syllabary from table
  Syllabary syllabary;
  if (!table_->Load() || !table_->GetSyllabary(&syllabary) || syllabary.empty())
    return false;
  // TODO: spelling algebra
  if (!schema_file.empty()) {
    
  }
  // build prism
  {
    prism_->Remove();
    if (!prism_->Build(syllabary, dict_file_checksum, schema_file_checksum) ||
        !prism_->Save()) {
      return false;
    }
  }
  return true;
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
  Config *config = schema->config();
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified in schema '%s'.",
                  schema->schema_id().c_str());
    return NULL;
  }
  std::string prism_name;
  if (!config->GetString("translator/prism", &prism_name)) {
    // usually same with dictionary name; different for alternative spelling
    prism_name = dict_name;
  }
  // obtain prism and table objects
  boost::filesystem::path path(ConfigDataManager::instance().user_data_dir());
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
