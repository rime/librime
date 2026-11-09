//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENTRY_COLLECTOR_H_
#define RIME_ENTRY_COLLECTOR_H_

#include <queue>
#include <string_view>
#include <boost/version.hpp>
#if BOOST_VERSION >= 108100
#include <boost/unordered/unordered_flat_map.hpp>
#endif
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

// For the sake of memory usage, don't use word -> { code -> weight } as there
// may be many words, but may not be many homographs for a word.
// The first encoding is stored inline to avoid one heap allocation per word;
// homographs are rare.
struct WordEntries {
  bool initialized = false;
  string code;
  double weight = 0.0;
  vector<pair<string, double>> homographs;
};

#if BOOST_VERSION >= 108100
template <class K, class V>
using WordHashMap = boost::unordered_flat_map<K, V>;
#else
template <class K, class V>
using WordHashMap = hash_map<K, V>;
#endif

using WordMap = WordHashMap<string, WordEntries>;
// [ (word, weight), ... ]
using EncodeQueue = std::queue<pair<string, string>>;

class PresetVocabulary;
class DictSettings;

class EntryCollector : public PhraseCollector {
 public:
  Syllabary syllabary;
  bool build_syllabary = true;
  vector<RawDictEntry> entries;
  size_t num_entries = 0;
  ReverseLookupTable stems;

 public:
  EntryCollector();
  explicit EntryCollector(Syllabary&& fixed_syllabary);
  virtual ~EntryCollector();

  void Configure(DictSettings* settings);
  // Collect all tables in one batch; temporary encoding data is released after.
  void Collect(const vector<path>& dict_files);

  // export contents of table and prism to text files
  void Dump(const path& file_path) const;

  void CreateEntry(const string& word,
                   const string& code_str,
                   const string& weight_str);
  bool TranslateWord(const string& word, vector<string>* code);

 protected:
  void LoadPresetVocabulary(DictSettings* settings);
  // call Collect() multiple times for all required tables
  void Collect(const path& dict_file);
  // encode all collected entries
  void Finish();

 protected:
  the<PresetVocabulary> preset_vocabulary;
  the<Encoder> encoder;
  EncodeQueue encode_queue;
  set<string /* word */> collection;
  WordMap words;

 private:
  void CreateEntryImpl(std::string_view word,
                       std::string_view code_str,
                       std::string_view weight_str);

 private:
  string current_dict_file;
  size_t line_number;
};

}  // namespace rime

#endif  // RIME_ENTRY_COLLECTOR_H_
