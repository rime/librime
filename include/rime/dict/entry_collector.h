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
#include <rime/algo/encoder.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/vocabulary.h>

namespace rime {

struct RawDictEntry {
  RawCode raw_code;
  std::string text;
  double weight;
};

// code -> weight
typedef std::map<std::string, double> WeightMap;
// word -> { code -> weight }
typedef std::map<std::string, WeightMap> WordMap;
// [ (word, weight), ... ]
typedef std::queue<std::pair<std::string, std::string> > EncodeQueue;

class PresetVocabulary;
class DictSettings;

class EntryCollector : public PhraseCollector {
 public:
  Syllabary syllabary;
  std::vector<RawDictEntry> entries;
  size_t num_entries;
  ReverseLookupTable stems;

 public:
  EntryCollector();
  ~EntryCollector();

  void Configure(DictSettings* settings);
  void Collect(const std::vector<std::string>& dict_files);

  // export contents of table and prism to text files
  void Dump(const std::string& file_name) const;

  void CreateEntry(const std::string &word,
                   const std::string &code_str,
                   const std::string &weight_str);
  bool TranslateWord(const std::string& word,
                     std::vector<std::string>* code);
 protected:
  void LoadPresetVocabulary(DictSettings* settings);
  // call Collect() multiple times for all required tables
  void Collect(const std::string &dict_file);
  // encode all collected entries
  void Finish();

 protected:
  scoped_ptr<PresetVocabulary> preset_vocabulary;
  scoped_ptr<Encoder> encoder;
  EncodeQueue encode_queue;
  std::set<std::string/* word */> collection;
  WordMap words;
  WeightMap total_weight;
};

}  // namespace rime

#endif  // RIME_ENTRY_COLLECTOR_H_
