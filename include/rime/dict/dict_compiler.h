// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICT_COMPILER_H_
#define RIME_DICT_COMPILER_H_

#include <string>
#include <rime/common.h>

namespace rime {

class Dictionary;
class Prism;
class Table;
class TreeDb;

class DictCompiler {
 public:
  DictCompiler(Dictionary *dictionary);
  
  bool Compile(const std::string &dict_file, const std::string &schema_file);

 private:
  bool BuildTable(const std::string &dict_file, uint32_t checksum);
  bool BuildPrism(const std::string &schema_file,
                  uint32_t dict_file_checksum, uint32_t schema_file_checksum);
  bool BuildReverseLookupDict(TreeDb *db, uint32_t dict_file_checksum);

  std::string dict_name_;
  shared_ptr<Prism> prism_;
  shared_ptr<Table> table_;
};

}  // namespace rime

#endif  // RIME_DICT_COMPILER_H_
