// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#include <rime/prism.h>
#include <queue>

namespace {

struct node_t {
  std::string key;
  size_t node_pos;
  node_t(const std::string& k, size_t pos) : key(k), node_pos(pos){
  }
};

}  // namespace

namespace rime {

void Prism::Load(const std::string &file){
  EZLOGGERPRINT("Load file: %s", file.c_str());
  trie_->open(file.c_str());
}

void Prism::Save(const std::string &file){
  EZLOGGERPRINT("Save file: %s", file.c_str());
  trie_->save(file.c_str());
}

// keys should be in order
void Prism::Build(const std::vector<std::string> &keys){
  size_t key_size = keys.size();
  std::vector<const char *> char_keys(key_size);
  
  size_t key_id = 0;
  for (std::vector<std::string>::const_iterator it = keys.begin();
      it != keys.end(); ++it, ++key_id) {
    char_keys[key_id] = it->c_str();
  }  
  trie_->build(key_size, &char_keys[0]);
}

bool Prism::HasKey(const std::string &key){
  Darts::DoubleArray::value_type value;
  trie_->exactMatchSearch(key.c_str(), value);
  return value != -1;
}

bool Prism::GetValue(const std::string &key, int *value){
  Darts::DoubleArray::result_pair_type result;
  trie_->exactMatchSearch(key.c_str(), result);
  
  if(result.value == -1)
    return false;
    
  *value = result.value;
  return true;
}

// Given a key, search all the keys in the tree which share a common prefix with that key.
void Prism::CommonPrefixSearch(const std::string &key, std::vector<int> *result, size_t limit){
  Darts::DoubleArray::result_pair_type result_pair[limit];
  size_t results = trie_->commonPrefixSearch(key.c_str(), result_pair, limit, key.length());
  results = std::min(results, limit);
  for(size_t i = 0; i < results; ++i){
    result->push_back(result_pair[i].value);
  }
}

void Prism::ExpandSearch(const std::string &key, std::vector<int> *result, size_t limit){
  if (limit == 0)
    return;
  size_t node_pos = 0;
  size_t key_pos = 0;
  int ret = trie_->traverse(key.c_str(), node_pos, key_pos);
  //key is not a valid path
  if(ret == -2)
    return;
  size_t count = 0;  
  std::queue<node_t> q;
  q.push(node_t(key, node_pos));
  while(!q.empty()){
    node_t node = q.front();
    q.pop();
    for(char ch = 'a'; ch <= 'z'; ++ch){
      std::string k = node.key + ch;
      size_t k_pos = node.key.length();
      size_t n_pos = node.node_pos;
      ret = trie_->traverse(k.c_str(), n_pos, k_pos);
      if(ret <= -2){
        //ignore
      }
      else if(ret == -1){
        q.push(node_t(k, n_pos));
      }
      else{
        q.push(node_t(k, n_pos));
        result->push_back(ret);
        if(++count > limit)
          return;
      }
    }
  }
}

size_t Prism::size()const{
  return trie_->size();
}

}  // namespace rime
