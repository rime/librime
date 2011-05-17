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
  void Build(const std::vector<std::string> &keys, const std::vector<int> &values);
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  std::size_t size()const;
  
private:
  scoped_ptr<Darts::DoubleArray> dtrie_;
};

}  // namespace rime

#endif  // RIME_DTRIE_H_