//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-07-04 GONG Chen <chen.sst@gmail.com>
//

#include <sstream>
#include <rime/common.h>
#include <rime/dict/string_table.h>

namespace rime {

StringTable::StringTable(const char* ptr, size_t size) {
  std::stringstream stream;
  stream.write(ptr, size);
  stream >> trie_;
}

bool StringTable::HasKey(string_view key) {
  marisa::Agent agent;
  agent.set_query(key.data(), key.length());
  return trie_.lookup(agent);
}

StringId StringTable::Lookup(string_view key) {
  marisa::Agent agent;
  agent.set_query(key.data(), key.length());
  if (trie_.lookup(agent)) {
    return agent.key().id();
  } else {
    return kInvalidStringId;
  }
}

void StringTable::CommonPrefixMatch(string_view query,
                                    vector<StringId>* result) {
  marisa::Agent agent;
  agent.set_query(query.data(), query.length());
  result->clear();
  while (trie_.common_prefix_search(agent)) {
    result->push_back(agent.key().id());
  }
}

void StringTable::Predict(string_view query, vector<StringId>* result) {
  marisa::Agent agent;
  agent.set_query(query.data(), query.length());
  result->clear();
  while (trie_.predictive_search(agent)) {
    result->push_back(agent.key().id());
  }
}

string StringTable::GetString(StringId string_id) {
  marisa::Agent agent;
  agent.set_query(string_id);
  try {
    trie_.reverse_lookup(agent);
  } catch (const marisa::Exception& /*ex*/) {
    LOG(ERROR) << "invalid id for string table: " << string_id;
    return string();
  }
  return string(agent.key().ptr(), agent.key().length());
}

size_t StringTable::NumKeys() const {
  return trie_.size();
}

size_t StringTable::BinarySize() const {
  return trie_.io_size();
}

void StringTableBuilder::Add(string_view key,
                             double weight,
                             StringId* reference) {
  keys_.push_back(key.data(), key.length(), (float)weight);
  references_.push_back(reference);
}

void StringTableBuilder::Clear() {
  trie_.clear();
  keys_.clear();
  references_.clear();
}

void StringTableBuilder::Build() {
  trie_.build(keys_);
  UpdateReferences();
}

void StringTableBuilder::UpdateReferences() {
  if (keys_.size() != references_.size()) {
    return;
  }
  marisa::Agent agent;
  for (size_t i = 0; i < keys_.size(); ++i) {
    if (references_[i]) {
      *references_[i] = keys_[i].id();
    }
  }
}

void StringTableBuilder::Dump(char* ptr, size_t size) {
  if (size < BinarySize()) {
    LOG(ERROR) << "insufficient memory to dump string table.";
    return;
  }

  std::stringstream stream;
  stream << trie_;
  stream.read(ptr, size);
}

}  // namespace rime
