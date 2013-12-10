//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_VOCABULARY_H_
#define RIME_VOCABULARY_H_

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

using Syllabary = std::set<std::string>;

class Code : public std::vector<int> {
 public:
  static const size_t kIndexCodeMaxLength = 3;

  bool operator< (const Code& other) const;
  bool operator== (const Code& other) const;

  void CreateIndex(Code* index_code);
};

struct DictEntry {
  std::string text;
  std::string comment;
  std::string preedit;
  double weight = 0.0;
  int commit_count = 0;
  Code code;  // multi-syllable code from prism
  std::string custom_code;  // user defined code
  int remaining_code_length = 0;

  DictEntry() = default;
  bool operator< (const DictEntry& other) const;
};

class DictEntryList : public std::vector<shared_ptr<DictEntry>> {
 public:
  void Sort();
  void SortRange(size_t start, size_t count);
};

using DictEntryFilter = std::function<bool (shared_ptr<DictEntry> entry)>;

class DictEntryFilterBinder {
 public:
  void AddFilter(DictEntryFilter filter);

 protected:
  DictEntryFilter filter_;
};

class Vocabulary;

struct VocabularyPage {
  DictEntryList entries;
  shared_ptr<Vocabulary> next_level;
};

class Vocabulary : public std::map<int, VocabularyPage> {
 public:
  DictEntryList* LocateEntries(const Code& code);
  void SortHomophones();
};

// word -> { code, ... }
using ReverseLookupTable = std::map<std::string, std::set<std::string>>;

}  // namespace rime

#endif  // RIME_VOCABULARY_H_
