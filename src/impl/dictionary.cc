// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <list>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
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
  prism_.reset(new Prism(name_ + ".prism.bin"));
  table_.reset(new Table(name_ + ".table.bin"));
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
  BOOST_FOREACH(const TableQueryResult::value_type &v, result) {
    int end_pos = v.first;
    BOOST_FOREACH(const TableAccessor &a, v.second) {
      if (a.extra_code()) {
        // TODO:
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
  DictEntryIterator result;
  BOOST_FOREACH(Prism::Match &match, keys) {
    int syllable_id = match.value;
    const TableAccessor a(table_->QueryWords(syllable_id));
    if (!a.exhausted()) {
        result.push_back(dictionary::Chunk(a));
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
    parser.GetNextDocument(doc);
  }
  if (doc.Type() != YAML::NodeType::Map) {
    return false;
  }
  std::string dict_name;
  std::string dict_version;
  {
    const YAML::Node *name_node = doc.FindValue("name");
    const YAML::Node *version_node = doc.FindValue("version");
    if (!name_node || !version_node) {
      return false;
    }
    *name_node >> dict_name;
    *version_node >> dict_version;
  }
  EZLOGGERVAR(dict_name);
  EZLOGGERVAR(dict_version);
  // read entries
  const YAML::Node *entries = doc.FindValue("entries");
  if (!entries ||
      entries->Type() != YAML::NodeType::Sequence) {
    return false;
  }

  dictionary::RawDictEntryList raw_entries;
  int entry_count = 0;
  Syllabary syllabary;
  for (YAML::Iterator it = entries->begin(); it != entries->end(); ++it) {
    if (it->Type() != YAML::NodeType::Sequence) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    // read a dict entry
    std::string word;
    std::string str_code;
    double weight = 1.0;
    if (it->size() < 2) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    (*it)[0] >> word;
    (*it)[1] >> str_code;
    if (it->size() > 2) {
      (*it)[2] >> weight;
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
    vocabulary.SortHomophones();
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
