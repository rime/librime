//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENTRY_COLLECTOR_H_
#define RIME_ENTRY_COLLECTOR_H_

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/vocabulary.h>

namespace rime {

class PresetVocabulary;
struct DictSettings;

struct EntryCollector {
  scoped_ptr<PresetVocabulary> preset_vocabulary;
  Syllabary syllabary;
  std::vector<dictionary::RawDictEntry> entries;
  size_t num_entries;
  std::queue<std::pair<std::string, std::string> > encode_queue;
  typedef std::map<std::string, double> WeightMap;
  std::map<std::string, WeightMap> words;
  WeightMap total_weight_for_word;
  std::set<std::string> collection;

  EntryCollector();
  ~EntryCollector();
  void LoadPresetVocabulary(DictSettings* settings);
  void Collect(const std::string &dict_file);
  void CreateEntry(const std::string &word,
                   const std::string &code_str,
                   const std::string &weight_str);
  bool Encode(const std::string &phrase, const std::string &weight_str,
              size_t start_pos, dictionary::RawCode *code);
};

}  // namespace rime

#endif  // RIME_ENTRY_COLLECTOR_H_
