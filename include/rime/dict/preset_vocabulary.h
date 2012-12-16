//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef PRESET_VOCABULARY_H_
#define PRESET_VOCABULARY_H_

#include <string>
#include <rime/common.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4351)
#endif
#include <kchashdb.h>
#if defined(_MSC_VER)
#pragma warning(default: 4351)
#pragma warning(default: 4244)
#endif

namespace rime {

class Dictionary;
class Prism;
class Table;
class TreeDb;

class PresetVocabulary {
 public:
  static PresetVocabulary *Create();
  // random access
  bool GetWeightForEntry(const std::string &key, double *weight);
  // traversing
  void Reset();
  bool GetNextEntry(std::string *key, std::string *value);
  bool IsQualifiedPhrase(const std::string& phrase,
                         const std::string& weight_str);
  
  void set_max_phrase_length(int length) { max_phrase_length_ = length; }
  void set_min_phrase_weight(double weight) { min_phrase_weight_ = weight; }

 protected:
  PresetVocabulary(const shared_ptr<kyotocabinet::TreeDB>& db);
  
  shared_ptr<kyotocabinet::TreeDB> db_;
  scoped_ptr<kyotocabinet::DB::Cursor> cursor_;
  int max_phrase_length_;
  double min_phrase_weight_;
};

}  // namespace rime

#endif  // PRESET_VOCABULARY_H_
