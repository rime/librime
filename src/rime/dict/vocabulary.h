//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_VOCABULARY_H_
#define RIME_VOCABULARY_H_

#include <stdint.h>
#include <rime_api.h>
#include <rime/common.h>

namespace rime {

using Syllabary = set<string>;

using SyllableId = int32_t;

class Code : public vector<SyllableId> {
 public:
  static const size_t kIndexCodeMaxLength = 3;

  bool operator< (const Code& other) const;
  bool operator== (const Code& other) const;

  void CreateIndex(Code* index_code);

  string ToString() const;
};

struct DictEntry {
  string text;
  string comment;
  string preedit;
  double weight = 0.0;
  int commit_count = 0;
  Code code;  // multi-syllable code from prism
  string custom_code;  // user defined code
  int remaining_code_length = 0;

  DictEntry() = default;
  bool operator< (const DictEntry& other) const;
};

class DictEntryList : public vector<of<DictEntry>> {
 public:
  void Sort();
  void SortRange(size_t start, size_t count);
};

using DictEntryFilter = function<bool (an<DictEntry> entry)>;

class DictEntryFilterBinder {
 public:
  virtual ~DictEntryFilterBinder() = default;
  RIME_API virtual void AddFilter(DictEntryFilter filter);

 protected:
  DictEntryFilter filter_;
};

class Vocabulary;

struct VocabularyPage {
  DictEntryList entries;
  an<Vocabulary> next_level;
};

class Vocabulary : public map<int, VocabularyPage> {
 public:
  DictEntryList* LocateEntries(const Code& code);
  void SortHomophones();
};

// word -> { code, ... }
using ReverseLookupTable = map<string, set<string>>;

}  // namespace rime

#endif  // RIME_VOCABULARY_H_
