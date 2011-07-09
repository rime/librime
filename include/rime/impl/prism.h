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

#include <set>
#include <string>
#include <vector>
#include <darts.h>
#include <rime/common.h>
#include <rime/impl/mapped_file.h>

namespace rime {

class Prism : public MappedFile {
 public:
  Prism(const std::string &file_name)
      : MappedFile(file_name), trie_(new Darts::DoubleArray) {}
  
  bool Load();
  bool Save();
  bool Build(const std::set<std::string> &keys);
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  void CommonPrefixSearch(const std::string &key, std::vector<int> *result);
  void ExpandSearch(const std::string &key, std::vector<int> *result, size_t limit);
  size_t size()const;
  
 private:
  scoped_ptr<Darts::DoubleArray> trie_;
};

}  // namespace rime

#endif  // RIME_PRISM_H_
