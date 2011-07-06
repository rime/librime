// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICTIONARY_H_
#define RIME_DICTIONRAY_H_

#include <string>
#include <rime/common.h>

namespace rime {

class Prism;
class Table;

class Dictionary {
 public:
  Dictionary(const std::string &name);
  virtual ~Dictionary();

  bool Compile(const std::string &source_file);
  bool Load();
  bool Unload();
  
  void* Lookup(const std::string &code);

 private:
  std::string name_;
  bool loaded_;
  scoped_ptr<Prism> prism_;
  scoped_ptr<Table> table_;
};

}  // namespace rime

#endif  // RIME_DICTIONRAY_H_
