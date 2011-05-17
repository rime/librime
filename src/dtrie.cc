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

//Peculiar logic means to comform to the build signature.
//Call the method to build a static trie.
void TrieMap::Build(const std::vector<std::string> &keys, const std::vector<int> &values){
  std::size_t key_size = keys.size();
  std::size_t value_size = values.size();
  assert(key_size == value_size);
  
  std::vector<std::size_t> lengths(key_size);
  std::vector<const char *> char_keys(key_size);
  
  std::size_t key_id = 0;
  for (std::vector<std::string>::const_iterator it = keys.begin();
      it != keys.end(); ++it, ++key_id) {
    char_keys[key_id] = it->c_str();
    lengths[key_id] = it->length();
  }
  
  dtrie_->build(key_size, &char_keys[0], &lengths[0], &values[0]);
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

std::size_t TrieMap::size()const{
  return dtrie_->size();
}

}  // namespace rime