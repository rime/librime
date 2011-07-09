// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <list>
#include <set>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/prism.h>
#include <rime/impl/table.h>

namespace rime {

const std::string CodeSequence::ToString() const {
  return boost::join(*this, " ");
}

void CodeSequence::FromString(const std::string &code) {
  boost::split(*dynamic_cast<std::vector<std::string> *>(this),
               code,
               boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
}

DictEntryIterator::DictEntryIterator() {
}

DictEntryIterator::operator bool() const {
  return !chunks_.empty();
}

shared_ptr<DictEntry> DictEntryIterator::operator->() {
  if (chunks_.empty()) {
    return shared_ptr<DictEntry>();
  }
  if (!entry_) {
    const std::pair<CodeSequence, TableEntryIterator> &p(chunks_.front());
    entry_.reset(new DictEntry);
    entry_->codes = p.first;
    entry_->text = p.second->text.c_str();
    entry_->weight = p.second->weight;
  }
  return entry_;
}

DictEntryIterator& DictEntryIterator::operator++() {
  if (!chunks_.empty()) {
    if (++chunks_.front().second == TableEntryIterator()) {
      chunks_.pop_front();
    }
  }
  entry_.reset();
  return *this;
}

void DictEntryIterator::AddChunk(const CodeSequence &codes,
                                 const TableEntryIterator &table_entry_iter) {
  EZLOGGERPRINT("chunk: %s|%s, ...",
                codes.ToString().c_str(),
                table_entry_iter->text.c_str());
  chunks_.push_back(std::make_pair(codes, table_entry_iter));
}

Dictionary::Dictionary(const std::string &name)
    : name_(name), loaded_(false) {

}

Dictionary::~Dictionary() {
  if (loaded_) {
    Unload();
  }
}

DictEntryIterator Dictionary::Lookup(const std::string &code) {
  EZLOGGERVAR(code);
  DictEntryIterator result;
  if (!loaded_)
    return result;
  std::vector<int> keys;
  prism_->CommonPrefixSearch(code, &keys);
  EZLOGGERPRINT("found %u keys thru the prism.", keys.size());
  CodeSequence codes;
  codes.resize(1);
  BOOST_FOREACH(int &syllable_id, keys) {
    codes[0] = table_->GetSyllable(syllable_id);
    const TableEntryVector *vec = table_->GetEntries(syllable_id);
    if (vec) {
      result.AddChunk(codes, vec->begin());
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
  Syllabary syllabary;
  int entry_count = 0;
  typedef std::list<rime::shared_ptr<DictEntry> > RawDictData;
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
    CodeSequence syllables;
    syllables.FromString(code);
    BOOST_FOREACH(const std::string &s, syllables) {
      if (syllabary.find(s) == syllabary.end())
        syllabary.insert(s);
    }
    shared_ptr<DictEntry> entry(new DictEntry);
    entry->codes.swap(syllables);
    entry->text.swap(word);
    entry->weight = weight;
    data.push_back(entry);
    ++entry_count;
  }
  EZLOGGERVAR(entry_count);
  EZLOGGERVAR(syllabary.size());
  // build prism
  {
    prism_.reset(new Prism(name_ + ".prism.bin"));
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
    BOOST_FOREACH(const shared_ptr<DictEntry> &e, data) {
      Code code;
      Code index_code;
      size_t k = 0;
      for (CodeSequence::const_iterator it = e->codes.begin();
           it != e->codes.end(); ++it) {
        int syllable_id = syllable_to_id[*it];
        code.push_back(syllable_id);
        if (k++ < Code::kIndexCodeMaxLength) {
          index_code.push_back(syllable_id);
        }
      }
      EntryDefinitionList &ls(vocabulary[index_code]);
      ls.resize(ls.size() + 1);
      EntryDefinition &d(ls.back());
      d.text.swap(e->text);
      d.weight = e->weight;
      d.code.swap(code);
    }
    // sort each group of homophones
    BOOST_FOREACH(Vocabulary::value_type &v, vocabulary) {
      std::stable_sort(v.second.begin(), v.second.end());
    }
    table_.reset(new Table(name_ + ".table.bin"));
    table_->Remove();
    if (!table_->Build(syllabary, vocabulary, data.size()) ||
        !table_->Save()) {
      return false;
    }  
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
