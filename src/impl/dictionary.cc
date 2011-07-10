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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/impl/dictionary.h>

namespace {

struct RawDictEntry {
  rime::RawCode raw_code;
  std::string text;
  double weight;
};

typedef std::list<rime::shared_ptr<RawDictEntry> > RawDictEntryList;

}  // namespace

namespace rime {

const std::string RawCode::ToString() const {
  return boost::join(*this, " ");
}

void RawCode::FromString(const std::string &code) {
  boost::split(*dynamic_cast<std::vector<std::string> *>(this),
               code,
               boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
}

bool Code::operator< (const Code &other) const {
  if (size() != other.size())
    return size() < other.size();
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return at(i) < other.at(i);
  }
  return false;
}

bool Code::operator== (const Code &other) const {
  if (size() != other.size())
    return false;
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return false;
  }
  return true;
}

void Code::CreateIndex(Code *index_code) {
  if (!index_code)
    return;
  size_t index_code_size = Code::kIndexCodeMaxLength;
  if (size() <= index_code_size) {
    index_code_size = size();
  }
  index_code->resize(index_code_size);
  std::copy(begin(),
            begin() + index_code_size,
            index_code->begin());
}

bool DictEntry::operator< (const DictEntry& other) const {
  // Sort different entries sharing the same code by weight desc.
  if (weight != other.weight)
    return weight > other.weight;
  return text < other.text;
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
    const std::pair<Code, table::EntryIterator> &p(chunks_.front());
    entry_.reset(new DictEntry);
    entry_->code = p.first;
    entry_->text = p.second->text.c_str();
    entry_->weight = p.second->weight;
  }
  return entry_;
}

DictEntryIterator& DictEntryIterator::operator++() {
  if (!chunks_.empty()) {
    if (++chunks_.front().second == table::EntryIterator()) {
      chunks_.pop_front();
    }
  }
  entry_.reset();
  return *this;
}

void DictEntryIterator::AddChunk(const Code &code,
                                 const table::EntryIterator &table_entry_iter) {
  EZLOGGERPRINT("chunk: %s, ...",
                table_entry_iter->text.c_str());
  chunks_.push_back(std::make_pair(code, table_entry_iter));
}

Dictionary::Dictionary(const std::string &name)
    : name_(name), loaded_(false) {

}

Dictionary::~Dictionary() {
  if (loaded_) {
    Unload();
  }
}

DictEntryIterator Dictionary::Lookup(const std::string &str_code) {
  EZLOGGERVAR(str_code);
  DictEntryIterator result;
  if (!loaded_)
    return result;
  std::vector<int> keys;
  prism_->CommonPrefixSearch(str_code, &keys);
  EZLOGGERPRINT("found %u keys thru the prism.", keys.size());
  Code code;
  code.resize(1);
  BOOST_FOREACH(int &syllable_id, keys) {
    code[0] = syllable_id;
    const table::EntryVector *vec = table_->GetEntries(syllable_id);
    if (vec) {
      result.AddChunk(code, vec->begin());
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
  
  RawDictEntryList raw_entries;
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
    shared_ptr<RawDictEntry> e(new RawDictEntry);
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
    BOOST_FOREACH(const shared_ptr<RawDictEntry> &e, raw_entries) {
      Code code;
      BOOST_FOREACH(const std::string &s, e->raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      Code index_code;
      code.CreateIndex(&index_code);
      DictEntryList &ls(vocabulary[index_code]);
      ls.resize(ls.size() + 1);
      DictEntry &d(ls.back());
      d.code.swap(code);
      d.text.swap(e->text);
      d.weight = e->weight;
    }
    // sort each group of homophones
    BOOST_FOREACH(Vocabulary::value_type &v, vocabulary) {
      std::stable_sort(v.second.begin(), v.second.end());
    }
    table_.reset(new Table(name_ + ".table.bin"));
    table_->Remove();
    if (!table_->Build(syllabary, vocabulary, entry_count) ||
        !table_->Save()) {
      return false;
    }  
  }
  return true;
}

bool Dictionary::Decode(const Code &code, RawCode *result) {
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
