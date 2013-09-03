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
#include <boost/function.hpp>
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
  std::string text;
  std::string comment;
  std::string preedit;
  double weight;
  int commit_count;
  Code code;  // multi-syllable code from prism
  std::string custom_code;  // user defined code
  int remaining_code_length;

  DictEntry() : weight(0.0), commit_count(0), remaining_code_length(0) {}
  bool operator< (const DictEntry& other) const;
};

class DictEntryList : public std::vector<shared_ptr<DictEntry> > {
 public:
  void Sort();
  void SortN(size_t count);
};

typedef boost::function<bool (shared_ptr<DictEntry> entry)> DictEntryFilter;

class DictEntryFilterManager {
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
  DictEntryList* LocateEntries(const Code &code);
  void SortHomophones();
};

// word -> { code, ... }
typedef std::map<std::string, std::set<std::string> > ReverseLookupTable;

}  // namespace rime

#endif  // RIME_VOCABULARY_H_
