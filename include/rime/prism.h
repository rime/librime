// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_PRISM_H_
#define RIME_PRISM_H_

#include <rime/common.h>
#include <darts.h>

namespace rime {

//
class Prism {
public:
  Prism() : trie_(new Darts::DoubleArray){};
  
  void Load(const std::string &file);
  void Save(const std::string &file);
  void Build(const std::vector<std::string> &keys);
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  void CommonPrefixSearch(const std::string &key, std::vector<int> *result, size_t limit);
  void ExpandSearch(const std::string &key, std::vector<int> *result, size_t limit);
  size_t size()const;
  
private:
  scoped_ptr<Darts::DoubleArray> trie_;
};

}  // namespace rime

#endif  // RIME_DTRIE_H_
