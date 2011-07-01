// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#include <rime/prism.h>
#include <cstring>
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

bool Prism::Load(){
  EZLOGGERPRINT("Load file: %s", file_name().c_str());
  //trie_->open(file_name().c_str());

  if (IsOpen())
    Close();
  
  if (!OpenReadOnly()) {
    EZLOGGERPRINT("Error opening prism file '%s'.", file_name().c_str());
    return false;
  }
  std::pair<char *, size_t> image = file()->find<char>("DoubleArray");
  if (!image.second) {
    EZLOGGERPRINT("Double array image not found.");
    return false;
  }
  size_t array_size = image.second / trie_->unit_size();
  EZLOGGERPRINT("Found double array image of size %u.", array_size);
  trie_->set_array(image.first, array_size);
}

bool Prism::Save(){
  EZLOGGERPRINT("Save file: %s", file_name().c_str());
  //trie_->save(file_name().c_str());

  // save the array to managed mapped file
  size_t image_size = trie_->total_size();
  if (!image_size) {
    EZLOGGERPRINT("Error: the trie has not been constructed!");
    return false;
  }
  const size_t kReservedSize = 1024;
  if (!Create(image_size + kReservedSize)) {
    EZLOGGERPRINT("Error creating prism file '%s'.", file_name().c_str());
    return false;
  }
  char *image = file()->construct<char>("DoubleArray", std::nothrow)[image_size]();
  if (!image) {
    EZLOGGERPRINT("Error creating double array image.");
    return false;
  }
  std::memcpy(image, trie_->array(), image_size);

  // write metadata to identify the format and version
  {
    const char kFormat[] = "Rime::Prism/0.9";
    CharAllocator char_allocator(file()->get_segment_manager());
    String *format = file()->construct<String>("Format")(kFormat, char_allocator);
    if (!format) {
      EZLOGGERPRINT("Error writing metadata into file '%s'.", file_name().c_str());
      return false;
    }
  }

  Close();
  return ShrinkToFit();
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
