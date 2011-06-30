// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_DTRIE_H_
#define RIME_DTRIE_H_

#include <rime/common.h>
#include <darts.h>

namespace rime {

//
class TrieMap {
public:
  TrieMap() : dtrie_(new Darts::DoubleArray){};
  
  void Load(const std::string &file);
  void Save(const std::string &file);
  void Build(const std::vector<std::string> &keys);
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  void CommonPrefixSearch(const std::string &key, size_t limit, std::vector<int> *result);
  void ExpandSearch(const std::string &key, std::vector<int> *result, const size_t limit);
  std::size_t size()const;
  
private:
  scoped_ptr<Darts::DoubleArray> dtrie_;
};

struct node_t {
  std::string key;
  size_t node_pos;
  node_t(const std::string& k, size_t pos) : key(k), node_pos(pos){
  }
};

}  // namespace rime

#endif  // RIME_DTRIE_H_