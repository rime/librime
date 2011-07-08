// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <deque>
#include <set>
#include <string>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/prism.h>
#include <rime/impl/table.h>

namespace {

typedef std::vector<std::string> RawCodeSequence;

struct RawDictEntry {
  std::string text;
  RawCodeSequence codes;
  double weight;
};

typedef std::deque<rime::shared_ptr<RawDictEntry> > RawDictData;

}  // namespace

namespace rime {

Dictionary::Dictionary(const std::string &name)
    : name_(name), loaded_(false) {

}

Dictionary::~Dictionary() {
  if (loaded_) {
    Unload();
  }
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
  int entry_count = 0;
  std::set<std::string> syllabary;
  RawDictData data;
  for (YAML::Iterator it = entries->begin(); it != entries->end(); ++it) {
    if (it->Type() != YAML::NodeType::Sequence) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    // read a dict entry
    std::string word;
    std::string code;
    double weight = 1.0;
    if (it->size() < 2) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    (*it)[0] >> word;
    (*it)[1] >> code;
    if (it->size() > 2) {
      (*it)[2] >> weight;
    }
    RawCodeSequence syllables;
    boost::split(syllables, code,
                 boost::algorithm::is_space(),
                 boost::algorithm::token_compress_on);
    BOOST_FOREACH(const std::string &s, syllables) {
      if (syllabary.find(s) == syllabary.end())
        syllabary.insert(s);
    }
    shared_ptr<RawDictEntry> entry(new RawDictEntry);
    entry->text.swap(word);
    entry->codes.swap(syllables);
    entry->weight = weight;
    data.push_back(entry);
    ++entry_count;
  }
  EZLOGGERVAR(entry_count);
  EZLOGGERVAR(syllabary.size());
  // build prism
  std::vector<std::string> keys(syllabary.size());
  std::copy(syllabary.begin(), syllabary.end(), keys.begin());
  prism_.reset(new Prism(name_ + ".prism.bin"));
  prism_->Remove();
  if (!prism_->Build(keys) ||
      !prism_->Save()) {
    return false;
  }
  // build table
  std::map<std::string, int> syllable_to_id;
  int syllable_id = 0;
  BOOST_FOREACH(const std::string &s, syllabary) {
    syllable_to_id[s] = syllable_id++;
  }
  Vocabulary vocabulary;
  BOOST_FOREACH(const shared_ptr<RawDictEntry> &e, data) {
    EntryDefinition d;
    d.text.swap(e->text);
    d.weight = e->weight;
    for (RawCodeSequence::const_iterator it = e->codes.begin();
         it != e->codes.end(); ++it) {
      d.code.push_back(syllable_to_id[*it]);
    }
    Code index_code;
    for (size_t i = 0; i < d.code.size() && i < Code::kIndexCodeMaxLength; ++i) {
      index_code.push_back(d.code[i]);
    }
    vocabulary[index_code].push_back(d);
  }
  // sort each group of homophones
  BOOST_FOREACH(Vocabulary::value_type &v, vocabulary) {
    std::stable_sort(v.second.begin(), v.second.end());
  }
  table_.reset(new Table(name_ + ".table.bin"));
  table_->Remove();
  if (!table_->Build(vocabulary, syllabary.size(), data.size()) ||
      !table_->Save()) {
    return false;
  }  
  return true;
}

bool Dictionary::Load() {
  if (!prism_->Load() || !table_->Load()) {
    prism_.reset();
    table_.reset();
    loaded_ = false;
  }
  else {
    loaded_ = true;
  }
  return loaded_;
}

bool Dictionary::Unload() {
  prism_.reset();
  table_.reset();
  loaded_ = false;
  return true;
}

}  // namespace rime
