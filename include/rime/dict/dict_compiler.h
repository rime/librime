//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICT_COMPILER_H_
#define RIME_DICT_COMPILER_H_

#include <string>
#include <vector>
#include <boost/function.hpp>
#include <rime/common.h>

namespace rime {

class Dictionary;
class Prism;
class Table;
class ReverseDb;
class DictSettings;

// return found dict file path, otherwize return empty string
typedef boost::function<const std::string (const std::string& file_name)>
DictFileFinder;

class DictCompiler {
 public:
  enum Options {
    kRebuildPrism = 1,
    kRebuildTable = 2,
    kRebuild = kRebuildPrism | kRebuildTable,
    kDump = 4,
  };

  DictCompiler(Dictionary *dictionary,
               DictFileFinder finder = NULL);

  bool Compile(const std::string &schema_file);
  void set_options(int options) { options_ = options; }

 private:
  std::string FindDictFile(const std::string& dict_name);
  bool BuildTable(DictSettings* settings,
                  const std::vector<std::string>& dict_files,
                  uint32_t dict_file_checksum);
  bool BuildPrism(const std::string& schema_file,
                  uint32_t dict_file_checksum, uint32_t schema_file_checksum);
  bool BuildReverseLookupDict(ReverseDb* db, uint32_t dict_file_checksum);

  std::string dict_name_;
  shared_ptr<Prism> prism_;
  shared_ptr<Table> table_;
  int options_;
  DictFileFinder dict_file_finder_;
};

}  // namespace rime

#endif  // RIME_DICT_COMPILER_H_
