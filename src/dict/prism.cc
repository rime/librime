// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-16 Zou Xu <zouivex@gmail.com>
// 2012-01-26 GONG Chen <chen.sst@gmail.com>  spelling algebra support
//
#include <cstring>
#include <queue>
#include <boost/scoped_array.hpp>
#include <rime/algo/algebra.h>
#include <rime/dict/prism.h>

namespace {

struct node_t {
  std::string key;
  size_t node_pos;
  node_t(const std::string& k, size_t pos) : key(k), node_pos(pos) {
  }
};

const char kPrismFormat[] = "Rime::Prism/1.0";

}  // namespace

namespace rime {

bool Prism::Load() {
  EZLOGGERPRINT("Load file: %s", file_name().c_str());

  if (IsOpen())
    Close();

  if (!OpenReadOnly()) {
    EZLOGGERPRINT("Error opening prism file '%s'.", file_name().c_str());
    return false;
  }

  metadata_ = Find<prism::Metadata>(0);
  if (!metadata_) {
    EZLOGGERPRINT("Metadata not found.");
    return false;
  }
  char *array = metadata_->double_array.get();
  if (!array) {
    EZLOGGERPRINT("Double array image not found.");
    return false;
  }
  size_t array_size = metadata_->double_array_size;
  EZLOGGERPRINT("Found double array image of size %u.", array_size);
  trie_->set_array(array, array_size);

  // TODO:
  spelling_map_ = NULL;

  return true;
}

bool Prism::Save() {
  EZLOGGERPRINT("Save file: %s", file_name().c_str());
  if (!trie_->total_size()) {
    EZLOGGERPRINT("Error: the trie has not been constructed!");
    return false;
  }
  return ShrinkToFit();
}

bool Prism::Build(const Syllabary &syllabary,
                  const Script *script,
                  uint32_t dict_file_checksum,
                  uint32_t schema_file_checksum) {
  // building double-array trie
  size_t num_syllables = syllabary.size();
  size_t num_spellings = script ? script->size() : syllabary.size();
  std::vector<const char *> keys(num_spellings);
  size_t key_id = 0;
  size_t map_size = 0;
  if (script) {
    for (Script::const_iterator it = script->begin();
         it != script->end(); ++it, ++key_id) {
      keys[key_id] = it->first.c_str();
      map_size += it->second.size();
    }
  }
  else {
    for (Syllabary::const_iterator it = syllabary.begin();
         it != syllabary.end(); ++it, ++key_id) {
      keys[key_id] = it->c_str();
    }
  }
  if (0 != trie_->build(num_spellings, &keys[0])) {
    EZLOGGERPRINT("Error building double-array trie.");
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
    EZLOGGERPRINT("Error creating prism file '%s'.", file_name().c_str());
    return false;
  }
  // creating metadata
  prism::Metadata *metadata = Allocate<prism::Metadata>();
  if (!metadata) {
    EZLOGGERPRINT("Error creating metadata in file '%s'.", file_name().c_str());
    return false;
  }
  std::strncpy(metadata->format, kPrismFormat, prism::Metadata::kFormatMaxLength);
  metadata->dict_file_checksum = dict_file_checksum;
  metadata->schema_file_checksum = schema_file_checksum;
  metadata->num_syllables = num_syllables;
  metadata->num_spellings = num_spellings;
  metadata_ = metadata;
  // saving double-array image
  char *array = Allocate<char>(image_size);
  if (!array) {
    EZLOGGERPRINT("Error creating double-array image.");
    return false;
  }
  std::memcpy(array, trie_->array(), image_size);
  metadata->double_array = array;
  metadata->double_array_size = array_size;
  // building spelling map
  if (script) {
    std::map<std::string, prism::SyllableId> syllable_to_id;
    prism::SyllableId syll_id = 0;
    for (Syllabary::const_iterator it = syllabary.begin();
         it != syllabary.end(); ++it) {
      syllable_to_id[*it] = syll_id++;
    }
    prism::SpellingMap* spelling_map = CreateArray<prism::SpellingMapItem>(num_spellings);
    if (!spelling_map) {
      EZLOGGERPRINT("Error creating spelling map.");
      return false;
    }
    Script::const_iterator i;
    prism::SpellingMapItem* item;
    for (i = script->begin(), item = spelling_map->begin();
         i != script->end(); ++i, ++item) {
      size_t list_size = i->second.size();
      item->size = list_size;
      item->at = Allocate<prism::SpellingDescriptor>(list_size);
      if (!item->at) {
        EZLOGGERPRINT("Error creating spelling descriptors.");
        return false;
      }
      std::vector<Spelling>::const_iterator j;
      prism::SpellingDescriptor* desc;
      for (j = i->second.begin(), desc = item->begin();
           j != i->second.end(); ++j, ++desc) {
        desc->syllable_id = syllable_to_id[j->str];
        desc->type = static_cast<int32_t>(j->properties.type);
        desc->credibility = j->properties.credibility;
        if (!CopyString(j->properties.tips, &desc->tips)) {
          EZLOGGERPRINT("Error creating spelling properties.");
          return false;
        }
      }
    }
    metadata->spelling_map = spelling_map;
    spelling_map_ = spelling_map;
  }
  return true;
}

bool Prism::HasKey(const std::string &key) {
  Darts::DoubleArray::value_type value;
  trie_->exactMatchSearch(key.c_str(), value);
  return value != -1;
}

bool Prism::GetValue(const std::string &key, int *value) {
  Darts::DoubleArray::result_pair_type result;
  trie_->exactMatchSearch(key.c_str(), result);

  if (result.value == -1)
    return false;

  *value = result.value;
  return true;
}

// Given a key, search all the keys in the tree which share a common prefix with that key.
void Prism::CommonPrefixSearch(const std::string &key, std::vector<Match> *result) {
  if (!result || key.empty())
    return;
  size_t len = key.length();
  result->resize(len);
  size_t num_results = trie_->commonPrefixSearch(key.c_str(), &result->front(), len, len);
  result->resize(num_results);
}

void Prism::ExpandSearch(const std::string &key, std::vector<Match> *result, size_t limit) {
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
    {
      Match match;
      match.value = ret;
      match.length = key_pos;
      result->push_back(match);
    }
    if (limit && ++count >= limit)
      return;
  }
  std::queue<node_t> q;
  q.push(node_t(key, node_pos));
  while(!q.empty()) {
    node_t node = q.front();
    q.pop();
    // TODO: configurable alphabet
    for (char ch = 'a'; ch <= 'z'; ++ch) {
      std::string k = node.key + ch;
      size_t k_pos = node.key.length();
      size_t n_pos = node.node_pos;
      ret = trie_->traverse(k.c_str(), n_pos, k_pos);
      if (ret <= -2) {
        //ignore
      }
      else if (ret == -1) {
        q.push(node_t(k, n_pos));
      }
      else {
        q.push(node_t(k, n_pos));
        {
          Match match;
          match.value = ret;
          match.length = k_pos;
          result->push_back(match);
        }
        if (limit && ++count >= limit)
          return;
      }
    }
  }
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
