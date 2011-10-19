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
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/syllablizer.h>

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

int match_extra_code(const table::Code *extra_code, int depth,
                     const SyllableGraph &syll_graph, int current_pos) {
  if (!extra_code || depth >= extra_code->size)
    return current_pos;  // success
  if (current_pos >= syll_graph.interpreted_length)
    return 0;  // failure (possibly success for completion in the future)
  EdgeMap::const_iterator edges = syll_graph.edges.find(current_pos);
  if (edges == syll_graph.edges.end()) {
    return 0;
  }
  table::SyllableId current_syll_id = extra_code->at[depth];
  int best_match = 0;
  BOOST_FOREACH(const EndVertexMap::value_type &edge, edges->second) {
    int end_vertex_pos = edge.first;
    const SpellingMap &spellings(edge.second);
    if (spellings.find(current_syll_id) == spellings.end())
      continue;
    int match_end_pos = match_extra_code(extra_code, depth + 1,
                                         syll_graph, end_vertex_pos);
    if (!match_end_pos) continue;
    if (match_end_pos > best_match) best_match = match_end_pos;
  }
  return best_match;
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
    EZLOGGERPRINT("Creating temporary dict entry '%s'.", e.text.c_str());
    entry_->code = chunk.code;
    entry_->text = e.text.c_str();
    if (!chunk.remaining_code.empty()) {
      entry_->prompt = "~" + chunk.remaining_code;
    }
    entry_->weight = e.weight;
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
  entry_.reset();
  return !empty();
}

Dictionary::Dictionary(const std::string &name)
    : name_(name), loaded_(false) {
  // TODO:
  boost::filesystem::path path(ConfigComponent::shared_data_dir());
  path /= name_;
  prism_.reset(new Prism(path.string() + ".prism.bin"));
  table_.reset(new Table(path.string() + ".table.bin"));
}

Dictionary::~Dictionary() {
  if (loaded_) {
    Unload();
  }
}

shared_ptr<DictEntryCollector> Dictionary::Lookup(const SyllableGraph &syllable_graph, int start_pos) {
  if (!loaded_)
    return shared_ptr<DictEntryCollector>();
  TableQueryResult result;
  if (!table_->Query(syllable_graph, start_pos, &result)) {
    return shared_ptr<DictEntryCollector>();
  }
  shared_ptr<DictEntryCollector> collector(new DictEntryCollector);
  // copy result
  BOOST_FOREACH(TableQueryResult::value_type &v, result) {
    int end_pos = v.first;
    BOOST_FOREACH(TableAccessor &a, v.second) {
      if (a.extra_code()) {
        do {
          int actual_end_pos = dictionary::match_extra_code(a.extra_code(), 0,
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
  return collector;
}

DictEntryIterator Dictionary::LookupWords(const std::string &str_code, bool predictive) {
  EZLOGGERVAR(str_code);
  if (!loaded_)
    return DictEntryIterator();
  std::vector<Prism::Match> keys;
  if (predictive) {
    const size_t kExpandSearchLimit = 0;  // unlimited!
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
      EZLOGGERVAR(remaining_code);
      result.push_back(dictionary::Chunk(a, remaining_code));
    }
  }
  return result;
}

bool Dictionary::Compile(const std::string &source_file) {
  EZLOGGERFUNCTRACKER;
  YAML::Node doc;
  {
    std::ifstream fin(source_file.c_str());
    YAML::Parser parser(fin);
    if (!parser.GetNextDocument(doc)) {
      EZLOGGERPRINT("Error parsing yaml doc in '%s'.", source_file.c_str());
      return false;
    }
  }
  if (doc.Type() != YAML::NodeType::Map) {
    EZLOGGERPRINT("Error: invalid yaml doc in '%s'.", source_file.c_str());
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
      EZLOGGERPRINT("Error: incomplete dict info in '%s'.", source_file.c_str());
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
  dictionary::RawDictEntryList raw_entries;
  int entry_count = 0;
  Syllabary syllabary;
  {
    std::ifstream fin(source_file.c_str());
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
        EZLOGGERPRINT("Invalid entry %d.", entry_count);
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

      shared_ptr<dictionary::RawDictEntry> e(new dictionary::RawDictEntry);
      e->raw_code.FromString(str_code);
      BOOST_FOREACH(const std::string &s, e->raw_code) {
        if (syllabary.find(s) == syllabary.end())
          syllabary.insert(s);
      }
      e->text.swap(word);
      e->weight = weight;
      raw_entries.push_back(e);
      ++entry_count;
    }
  }
  EZLOGGERVAR(entry_count);
  EZLOGGERVAR(syllabary.size());
  // build prism
  {
    prism_->Remove();
    if (!prism_->Build(syllabary) ||
        !prism_->Save()) {
      return false;
    }
  }
  // build table
  {
    std::map<std::string, int> syllable_to_id;
    int syllable_id = 0;
    BOOST_FOREACH(const std::string &s, syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    Vocabulary vocabulary;
    BOOST_FOREACH(const shared_ptr<dictionary::RawDictEntry> &e, raw_entries) {
      Code code;
      BOOST_FOREACH(const std::string &s, e->raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      DictEntryList *ls = vocabulary.LocateEntries(code);
      if (!ls) {
        EZLOGGERPRINT("Error locating entries in vocabulary.");
        continue;
      }
      ls->resize(ls->size() + 1);
      DictEntry &d(ls->back());
      d.code.swap(code);
      d.text.swap(e->text);
      d.weight = e->weight;
    }
    if (sort_order != "original") {
      vocabulary.SortHomophones();
    }
    table_->Remove();
    if (!table_->Build(syllabary, vocabulary, entry_count) ||
        !table_->Save()) {
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
  if (!prism_->Load() || !table_->Load()) {
    EZLOGGERPRINT("Error loading dictionary '%s'.", name_.c_str());
    prism_->Close();
    table_->Close();
    loaded_ = false;
  }
  else {
    loaded_ = true;
  }
  return loaded_;
}

bool Dictionary::Unload() {
  prism_->Close();
  table_->Close();
  loaded_ = false;
  return true;
}

}  // namespace rime
