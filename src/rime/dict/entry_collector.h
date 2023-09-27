//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENTRY_COLLECTOR_H_
#define RIME_ENTRY_COLLECTOR_H_

#include <queue>
#include <rime/common.h>
#include <rime/algo/encoder.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/vocabulary.h>

namespace rime {

struct RawDictEntry {
  RawCode raw_code;
  string text;
  double weight;
};

// code -> weight
using WeightMap = map<string, double>;
// word -> [ { code, weight } ]
// For the sake of memory usage, don't use word -> { code -> weight } as there
// may be many words, but may not be many representations for a word
using WordMap = hash_map<string, vector<pair<string, double>>>;
// [ (word, weight), ... ]
using EncodeQueue = std::queue<pair<string, string>>;

class PresetVocabulary;
class DictSettings;

class EntryCollector : public PhraseCollector {
 public:
  Syllabary syllabary;
  bool build_syllabary = true;
  vector<of<RawDictEntry>> entries;
  size_t num_entries = 0;
  ReverseLookupTable stems;

 public:
  EntryCollector();
  explicit EntryCollector(Syllabary&& fixed_syllabary);
  virtual ~EntryCollector();

  void Configure(DictSettings* settings);
  void Collect(const vector<string>& dict_files);

  // export contents of table and prism to text files
  void Dump(const string& file_name) const;

  void CreateEntry(const string& word,
                   const string& code_str,
                   const string& weight_str);
  bool TranslateWord(const string& word, vector<string>* code);

 protected:
  void LoadPresetVocabulary(DictSettings* settings);
  // call Collect() multiple times for all required tables
  void Collect(const string& dict_file);
  // encode all collected entries
  void Finish();

 protected:
  the<PresetVocabulary> preset_vocabulary;
  the<Encoder> encoder;
  EncodeQueue encode_queue;
  set<string /* word */> collection;
  WordMap words;
  WeightMap total_weight;
};

}  // namespace rime

#endif  // RIME_ENTRY_COLLECTOR_H_
