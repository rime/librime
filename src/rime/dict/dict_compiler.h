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
class EntryCollector;
class Vocabulary;
class ResourceResolver;

class DictCompiler {
 public:
  enum Options {
    kRebuildPrism = 1,
    kRebuildTable = 2,
    kRebuild = kRebuildPrism | kRebuildTable,
    kDump = 4,
  };

  RIME_API explicit DictCompiler(Dictionary *dictionary);
  RIME_API virtual ~DictCompiler();

  RIME_API bool Compile(const string &schema_file);
  void set_options(int options) { options_ = options; }

 private:
  bool BuildTable(int table_index,
                  EntryCollector& collector,
                  DictSettings* settings,
                  const vector<string>& dict_files,
                  uint32_t dict_file_checksum);
  bool BuildPrism(const string& schema_file,
                  uint32_t dict_file_checksum,
                  uint32_t schema_file_checksum);
  bool BuildReverseDb(DictSettings* settings,
                      const EntryCollector& collector,
                      const Vocabulary& vocabulary,
                      uint32_t dict_file_checksum);

  const string& dict_name_;
  const vector<string>& packs_;
  an<Prism> prism_;
  an<EditDistanceCorrector> correction_;
  vector<of<Table>> tables_;
  int options_ = 0;
  the<ResourceResolver> source_resolver_;
  the<ResourceResolver> target_resolver_;
};

}  // namespace rime

#endif  // RIME_DICT_COMPILER_H_
