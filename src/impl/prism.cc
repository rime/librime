// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//
#include <cstring>
#include <queue>
#include <boost/scoped_array.hpp>
#include <rime/impl/prism.h>

namespace {

struct node_t {
  std::string key;
  size_t node_pos;
  node_t(const std::string& k, size_t pos) : key(k), node_pos(pos){
  }
};

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  int num_syllables;
  int num_spellings;
};

const char kPrismFormat[] = "Rime::Prism/0.9";

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
  return true;
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

  {
    Metadata *metadata = file()->construct<Metadata>("Metadata")();
    if (!metadata) {
      EZLOGGERPRINT("Error writing metadata into file '%s'.", file_name().c_str());
      return false;
    }
    std::strncpy(metadata->format, kPrismFormat, Metadata::kFormatMaxLength);
    // TODO:
    metadata->num_syllables = trie_->size();
    metadata->num_spellings = trie_->size();
  }

  char *image = file()->construct<char>("DoubleArray", std::nothrow)[image_size]();
  if (!image) {
    EZLOGGERPRINT("Error creating double array image.");
    return false;
  }
  std::memcpy(image, trie_->array(), image_size);

  Close();
  return ShrinkToFit();
}

bool Prism::Build(const Syllabary &keyset){
  size_t key_size = keyset.size();
  std::vector<const char *> char_keys(key_size);
  
  size_t key_id = 0;
  for (Syllabary::const_iterator it = keyset.begin();
       it != keyset.end(); ++it, ++key_id) {
    char_keys[key_id] = it->c_str();
  }  
  return 0 == trie_->build(key_size, &char_keys[0]);
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
void Prism::CommonPrefixSearch(const std::string &key, std::vector<int> *result){
  size_t len = key.length();
  boost::scoped_array<Darts::DoubleArray::result_pair_type> result_pair(
      new Darts::DoubleArray::result_pair_type[len]);
  size_t results = trie_->commonPrefixSearch(key.c_str(), result_pair.get(), len, len);
  for(size_t i = 0; i < results; ++i){
    result->push_back(result_pair[i].value);
  }
}

void Prism::ExpandSearch(const std::string &key, std::vector<int> *result, size_t limit){
  if (limit == 0)
    return;
  size_t node_pos = 0;
  size_t key_pos = 0;
  size_t count = 0;  
  int ret = trie_->traverse(key.c_str(), node_pos, key_pos);
  //key is not a valid path
  if(ret == -2)
    return;
  if(ret != -1) {
    result->push_back(ret);
    if (++count > limit)
      return;
  }
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
