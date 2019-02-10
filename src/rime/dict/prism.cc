//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-16 Zou Xu <zouivex@gmail.com>
// 2012-01-26 GONG Chen <chen.sst@gmail.com>  spelling algebra support
//
#include <cfloat>
#include <cstring>
#include <queue>
#include <rime/algo/algebra.h>
#include <rime/dict/prism.h>

namespace rime {

namespace {

struct node_t {
  string key;
  size_t node_pos;
};

}  // namespace

const char kPrismFormat[] = "Rime::Prism/3.0";

const char kPrismFormatPrefix[] = "Rime::Prism/";
const size_t kPrismFormatPrefixLen = sizeof(kPrismFormatPrefix) - 1;

const char kDefaultAlphabet[] = "abcdefghijklmnopqrstuvwxyz";

SpellingAccessor::SpellingAccessor(prism::SpellingMap* spelling_map,
                                   SyllableId spelling_id)
    : spelling_id_(spelling_id), iter_(NULL), end_(NULL) {
  if (spelling_map &&
      spelling_id < static_cast<SyllableId>(spelling_map->size)) {
    iter_ = spelling_map->at[spelling_id].begin();
    end_ = spelling_map->at[spelling_id].end();
  }
}

bool SpellingAccessor::Next() {
  if (exhausted())
    return false;
  if (!iter_ || ++iter_ >= end_)
      spelling_id_ = -1;
  return exhausted();
}

bool SpellingAccessor::exhausted() const {
  return spelling_id_ == -1;
}

SyllableId SpellingAccessor::syllable_id() const {
  if (iter_ && iter_ < end_)
    return iter_->syllable_id;
  else
    return spelling_id_;
}

SpellingProperties SpellingAccessor::properties() const {
  SpellingProperties props;
  if (iter_ && iter_ < end_) {
    props.type = static_cast<SpellingType>(iter_->type);
    props.credibility = iter_->credibility;
    if (!iter_->tips.empty())
      props.tips = iter_->tips.c_str();
  }
  return props;
}

Prism::Prism(const string& file_name)
  : MappedFile(file_name), trie_(new Darts::DoubleArray) {
}

bool Prism::Load() {
  LOG(INFO) << "loading prism file: " << file_name();

  if (IsOpen())
    Close();

  if (!OpenReadOnly()) {
    LOG(ERROR) << "error opening prism file '" << file_name() << "'.";
    return false;
  }

  metadata_ = Find<prism::Metadata>(0);
  if (!metadata_) {
    LOG(ERROR) << "metadata not found.";
    Close();
    return false;
  }
  if (strncmp(metadata_->format, kPrismFormatPrefix, kPrismFormatPrefixLen)) {
    LOG(ERROR) << "invalid metadata.";
    Close();
    return false;
  }
  format_ = atof(&metadata_->format[kPrismFormatPrefixLen]);

  char* array = metadata_->double_array.get();
  if (!array) {
    LOG(ERROR) << "double array image not found.";
    Close();
    return false;
  }
  size_t array_size = metadata_->double_array_size;
  LOG(INFO) << "found double array image of size " << array_size << ".";
  trie_->set_array(array, array_size);

  spelling_map_ = NULL;
  if (format_ > 1.0 - DBL_EPSILON) {
    spelling_map_ = metadata_->spelling_map.get();
  }
  return true;
}

bool Prism::Save() {
  LOG(INFO) << "saving prism file: " << file_name();
  if (!trie_->total_size()) {
    LOG(ERROR) << "the trie has not been constructed!";
    return false;
  }
  return ShrinkToFit();
}
bool Prism::Build(const Syllabary& syllabary,
                  const Script* script,
                  uint32_t dict_file_checksum,
                  uint32_t schema_file_checksum) {
  // building double-array trie
  size_t num_syllables = syllabary.size();
  size_t num_spellings = script ? script->size() : syllabary.size();
  vector<const char*> keys(num_spellings);
  size_t key_id = 0;
  size_t map_size = 0;
  if (script) {
    for (auto it = script->begin(); it != script->end(); ++it, ++key_id) {
      keys[key_id] = it->first.c_str();
      map_size += it->second.size();
    }
  }
  else {
    for (auto it = syllabary.begin(); it != syllabary.end(); ++it, ++key_id) {
      keys[key_id] = it->c_str();
    }
  }
  if (0 != trie_->build(num_spellings, &keys[0])) {
    LOG(ERROR) << "Error building double-array trie.";
    return false;
  }
  // creating prism file
  size_t array_size = trie_->size();
  size_t image_size = trie_->total_size();
  const size_t kDescriptorExtraSize = 12;
  size_t estimated_map_size = num_spellings * 12 +
      map_size * (4 + sizeof(prism::SpellingDescriptor) + kDescriptorExtraSize);
  const size_t kReservedSize = 1024;
  if (!Create(image_size + estimated_map_size + kReservedSize)) {
    LOG(ERROR) << "Error creating prism file '" << file_name() << "'.";
    return false;
  }
  // creating metadata
  auto metadata = Allocate<prism::Metadata>();
  if (!metadata) {
    LOG(ERROR) << "Error creating metadata in file '" << file_name() << "'.";
    return false;
  }
  metadata->dict_file_checksum = dict_file_checksum;
  metadata->schema_file_checksum = schema_file_checksum;
  metadata->num_syllables = num_syllables;
  metadata->num_spellings = num_spellings;
  metadata_ = metadata;
  // alphabet
  {
    set<char> alphabet;
    for (size_t i = 0; i < num_spellings; ++i)
      for (const char* p = keys[i]; *p; ++p)
        alphabet.insert(*p);
    char* p = metadata->alphabet;
    set<char>::const_iterator c = alphabet.begin();
    for (; c != alphabet.end(); ++p, ++c)
      *p = *c;
    *p = '\0';
  }
  // saving double-array image
  char* array = Allocate<char>(image_size);
  if (!array) {
    LOG(ERROR) << "Error creating double-array image.";
    return false;
  }
  std::memcpy(array, trie_->array(), image_size);
  metadata->double_array = array;
  metadata->double_array_size = array_size;
  // building spelling map
  if (script) {
    map<string, SyllableId> syllable_to_id;
    SyllableId syll_id = 0;
    for (auto it = syllabary.begin(); it != syllabary.end(); ++it) {
      syllable_to_id[*it] = syll_id++;
    }
    auto spelling_map = CreateArray<prism::SpellingMapItem>(num_spellings);
    if (!spelling_map) {
      LOG(ERROR) << "Error creating spelling map.";
      return false;
    }
    auto i = script->begin();
    auto item = spelling_map->begin();
    for (; i != script->end(); ++i, ++item) {
      size_t list_size = i->second.size();
      item->size = list_size;
      item->at = Allocate<prism::SpellingDescriptor>(list_size);
      if (!item->at) {
        LOG(ERROR) << "Error creating spelling descriptors.";
        return false;
      }
      auto j = i->second.begin();
      auto desc = item->begin();
      for (; j != i->second.end(); ++j, ++desc) {
        desc->syllable_id = syllable_to_id[j->str];
        desc->type = static_cast<int32_t>(j->properties.type);
        desc->credibility = j->properties.credibility;
        if (!j->properties.tips.empty() &&
            !CopyString(j->properties.tips, &desc->tips)) {
          LOG(ERROR) << "Error creating spelling properties.";
          return false;
        }
      }
    }
    metadata->spelling_map = spelling_map;
    spelling_map_ = spelling_map;
  }
  // at last, complete the metadata
  std::strncpy(metadata->format, kPrismFormat,
               prism::Metadata::kFormatMaxLength);
  return true;
}

bool Prism::HasKey(const string& key) {
  int value = trie_->exactMatchSearch<int>(key.c_str());
  return value != -1;
}

bool Prism::GetValue(const string& key, int* value) const {
  int result = trie_->exactMatchSearch<int>(key.c_str());
  if (result == -1) {
    return false;
  }
  *value = result;
  return true;
}

// Given a key, search all the keys in the tree that share
// a common prefix with that key.
void Prism::CommonPrefixSearch(const string& key,
                               vector<Match>* result) {
  if (!result || key.empty())
    return;
  size_t len = key.length();
  result->resize(len);
  size_t num_results = trie_->commonPrefixSearch(key.c_str(),
                                                 &result->front(), len, len);
  result->resize(num_results);
}

void Prism::ExpandSearch(const string& key,
                         vector<Match>* result,
                         size_t limit) {
  if (!result)
    return;
  result->clear();
  size_t count = 0;
  size_t node_pos = 0;
  size_t key_pos = 0;
  int ret = trie_->traverse(key.c_str(), node_pos, key_pos);
  //key is not a valid path
  if (ret == -2)
    return;
  if (ret != -1) {
    result->push_back(Match{ret, key_pos});
    if (limit && ++count >= limit)
      return;
  }
  std::queue<node_t> q;
  q.push({key, node_pos});
  while(!q.empty()) {
    node_t node = q.front();
    q.pop();
    const char* c = (format_ > 1.0 - DBL_EPSILON) ? metadata_->alphabet
                                                  : kDefaultAlphabet;
    for (; *c; ++c) {
      string k = node.key + *c;
      size_t k_pos = node.key.length();
      size_t n_pos = node.node_pos;
      ret = trie_->traverse(k.c_str(), n_pos, k_pos);
      if (ret <= -2) {
        //ignore
      }
      else if (ret == -1) {
        q.push({k, n_pos});
      }
      else {
        q.push({k, n_pos});
        result->push_back(Match{ret, k_pos});
        if (limit && ++count >= limit)
          return;
      }
    }
  }
}

SpellingAccessor Prism::QuerySpelling(SyllableId spelling_id) {
  return SpellingAccessor(spelling_map_, spelling_id);
}

size_t Prism::array_size() const {
  return trie_->size();
}

uint32_t Prism::dict_file_checksum() const {
  return metadata_ ? metadata_->dict_file_checksum : 0;
}

uint32_t Prism::schema_file_checksum() const {
  return metadata_ ? metadata_->schema_file_checksum : 0;
}

}  // namespace rime
