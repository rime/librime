//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICT_COMPILER_H_
#define RIME_DICT_COMPILER_H_

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class Dictionary;
class Prism;
class Table;
class ReverseDb;
class DictSettings;
class EditDistanceCorrector;

class DictCompiler {
 public:
  enum Options {
    kRebuildPrism = 1,
    kRebuildTable = 2,
    kRebuild = kRebuildPrism | kRebuildTable,
    kDump = 4,
  };

  RIME_API DictCompiler(Dictionary *dictionary, const string& prefix = "");

  RIME_API bool Compile(const string &schema_file);
  void set_options(int options) { options_ = options; }

 private:
  bool BuildTable(DictSettings* settings,
                  const vector<string>& dict_files,
                  uint32_t dict_file_checksum);
  bool BuildPrism(const string& schema_file,
                  uint32_t dict_file_checksum,
                  uint32_t schema_file_checksum);
  bool BuildReverseLookupDict(ReverseDb* db, uint32_t dict_file_checksum);

  string dict_name_;
  an<Prism> prism_;
  an<EditDistanceCorrector> correction_;
  an<Table> table_;
  int options_ = 0;
  string prefix_;
};

}  // namespace rime

#endif  // RIME_DICT_COMPILER_H_
