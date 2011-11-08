// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_VOCABULARY_H_
#define RIME_VOCABULARY_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

typedef std::set<std::string> Syllabary;

class Code : public std::vector<int> {
 public:
  static const size_t kIndexCodeMaxLength = 3;

  bool operator< (const Code &other) const;
  bool operator== (const Code &other) const;

  void CreateIndex(Code* index_code);
};

struct DictEntry {
  Code code;
  std::string text;
  std::string comment;
  double weight;
  int commit_count;

  DictEntry() : weight(0.0), commit_count(0) {}
  bool operator< (const DictEntry& other) const;
};

typedef std::vector<DictEntry> DictEntryList;

class Vocabulary;

struct VocabularyPage {
  DictEntryList entries;
  shared_ptr<Vocabulary> next_level;
};

class Vocabulary : public std::map<int, VocabularyPage> {
 public:
  DictEntryList* LocateEntries(const Code &code);
  void SortHomophones();
};

}  // namespace rime

#endif  // RIME_VOCABULARY_H_
