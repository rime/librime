//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef PRESET_VOCABULARY_H_
#define PRESET_VOCABULARY_H_

#include <string>
#include <rime/common.h>

namespace rime {

struct VocabularyDb;

class PresetVocabulary {
 public:
  PresetVocabulary();
  ~PresetVocabulary();

  // random access
  bool GetWeightForEntry(const std::string& key, double* weight);
  // traversing
  void Reset();
  bool GetNextEntry(std::string* key, std::string* value);
  bool IsQualifiedPhrase(const std::string& phrase,
                         const std::string& weight_str);

  void set_max_phrase_length(int length) { max_phrase_length_ = length; }
  void set_min_phrase_weight(double weight) { min_phrase_weight_ = weight; }

  static std::string DictFilePath();

 protected:
  scoped_ptr<VocabularyDb> db_;
  int max_phrase_length_;
  double min_phrase_weight_;
};

}  // namespace rime

#endif  // PRESET_VOCABULARY_H_
