// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#include <rime/dtrie.h>

namespace rime{

void TrieMap::Load(const std::string &file){
  EZLOGGERPRINT("Load file: %s", file.c_str());
  dtrie_->open(file.c_str());
}

void TrieMap::Save(const std::string &file){
  EZLOGGERPRINT("Save file: %s", file.c_str());
  dtrie_->save(file.c_str());
}

//keys vector shoulb in in order
void TrieMap::Build(const std::vector<std::string> &keys){
  std::size_t key_size = keys.size();
  std::vector<const char *> char_keys(key_size);
  
  std::size_t key_id = 0;
  for (std::vector<std::string>::const_iterator it = keys.begin();
      it != keys.end(); ++it, ++key_id) {
    char_keys[key_id] = it->c_str();
  }  
  dtrie_->build(key_size, &char_keys[0]);
}

bool TrieMap::HasKey(const std::string &key){
  Darts::DoubleArray::value_type value;
  dtrie_->exactMatchSearch(key.c_str(), value);
  return value != -1;
}

bool TrieMap::GetValue(const std::string &key, int *value){
  Darts::DoubleArray::result_pair_type result;
  dtrie_->exactMatchSearch(key.c_str(), result);
  
  if(result.value == -1)
    return false;
    
  *value = result.value;
  return true;
}

//Giving a key, search all the keys in the tree which share a common prefix with that key.
void TrieMap::CommonPrefixSearch(const std::string &key, size_t limit, std::vector<int> *result){
  Darts::DoubleArray::result_pair_type result_pair[limit];
  size_t results = dtrie_->commonPrefixSearch(key.c_str(), result_pair, limit, key.length());
  results = std::min(results, limit);
  for(size_t i = 0; i < results; ++i){
    result->push_back(result_pair[i].value);
  }
}

void TrieMap::ExpandSearch(const std::string &key, std::vector<std::string> *result, size_t limit){
  if( limit == 0)
    return;
  
}

std::size_t TrieMap::size()const{
  return dtrie_->size();
}

}  // namespace rime