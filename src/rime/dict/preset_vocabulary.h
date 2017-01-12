//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef PRESET_VOCABULARY_H_
#define PRESET_VOCABULARY_H_

#include <rime/common.h>

namespace rime {

struct VocabularyDb;

class PresetVocabulary {
 public:
  PresetVocabulary();
  ~PresetVocabulary();

  // random access
  bool GetWeightForEntry(const string& key, double* weight);
  // traversing
  void Reset();
  bool GetNextEntry(string* key, string* value);
  bool IsQualifiedPhrase(const string& phrase,
                         const string& weight_str);

  void set_max_phrase_length(int length) { max_phrase_length_ = length; }
  void set_min_phrase_weight(double weight) { min_phrase_weight_ = weight; }

  static string DictFilePath();

 protected:
  the<VocabularyDb> db_;
  int max_phrase_length_ = 0;
  double min_phrase_weight_ = 0.0;
};

}  // namespace rime

#endif  // PRESET_VOCABULARY_H_
