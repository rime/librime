// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-24 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/foreach.hpp>
#include <rime/impl/vocabulary.h>

namespace rime {

bool Code::operator< (const Code &other) const {
  if (size() != other.size())
    return size() < other.size();
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return at(i) < other.at(i);
  }
  return false;
}

bool Code::operator== (const Code &other) const {
  if (size() != other.size())
    return false;
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return false;
  }
  return true;
}

void Code::CreateIndex(Code *index_code) {
  if (!index_code)
    return;
  size_t index_code_size = Code::kIndexCodeMaxLength;
  if (size() <= index_code_size) {
    index_code_size = size();
  }
  index_code->resize(index_code_size);
  std::copy(begin(),
            begin() + index_code_size,
            index_code->begin());
}

bool DictEntry::operator< (const DictEntry& other) const {
  // Sort different entries sharing the same code by weight desc.
  if (weight != other.weight)
    return weight > other.weight;
  return text < other.text;
}

DictEntryList* Vocabulary::LocateEntries(const Code &code) {
  Vocabulary *v = this;
  size_t n = code.size();
  for (size_t i = 0; i < n; ++i) {
    int key = -1;
    if (i < Code::kIndexCodeMaxLength)
      key = code[i];
    VocabularyPage &page((*v)[key]);
    if (i == n - 1 || i == Code::kIndexCodeMaxLength) {
      return &page.entries;
    }
    else {
      if (!page.next_level) {
        page.next_level.reset(new Vocabulary);
      }
      v = page.next_level.get();
    }
  }
  return NULL;
}

void Vocabulary::SortHomophones() {
  BOOST_FOREACH(Vocabulary::value_type &v, *this) {
    VocabularyPage &page(v.second);
     std::stable_sort(page.entries.begin(), page.entries.end());
    if (page.next_level)
      page.next_level->SortHomophones();
  }
}

}  // namespace rime
