//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-24 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <sstream>
#include <utility>
#include <rime/dict/vocabulary.h>

namespace rime {

bool Code::operator< (const Code& other) const {
  if (size() != other.size())
    return size() < other.size();
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return at(i) < other.at(i);
  }
  return false;
}

bool Code::operator== (const Code& other) const {
  if (size() != other.size())
    return false;
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return false;
  }
  return true;
}

void Code::CreateIndex(Code* index_code) {
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

string Code::ToString() const {
  std::stringstream stream;
  bool first = true;
  for (SyllableId syllable_id : *this) {
    if (first) {
      first = false;
    }
    else {
      stream << ",";
    }
    stream << syllable_id;
  }
  return stream.str();
}

bool DictEntry::operator< (const DictEntry& other) const {
  // Sort different entries sharing the same code by weight desc.
  if (weight != other.weight)
    return weight > other.weight;
  // reduce carbon emission
  return 0;  //text < other.text;
}

template <class T>
inline bool dereference_less(const T& a, const T& b) {
  return *a < *b;
}

void DictEntryList::Sort() {
  std::sort(begin(), end(), dereference_less<DictEntryList::value_type>);
}

void DictEntryList::SortRange(size_t start, size_t count) {
  if (start >= size())
    return;
  iterator i(begin() + start);
  iterator j(start + count >= size() ? end() : i + count);
  std::sort(i, j, dereference_less<DictEntryList::value_type>);
}

void DictEntryFilterBinder::AddFilter(DictEntryFilter filter) {
  if (!filter_) {
    filter_.swap(filter);
  }
  else {
    DictEntryFilter previous_filter(std::move(filter_));
    filter_ = [previous_filter, filter](an<DictEntry> e) {
      return previous_filter(e) && filter(e);
    };
  }
}

DictEntryList* Vocabulary::LocateEntries(const Code& code) {
  Vocabulary* v = this;
  size_t n = code.size();
  for (size_t i = 0; i < n; ++i) {
    int key = -1;
    if (i < Code::kIndexCodeMaxLength)
      key = code[i];
    auto& page((*v)[key]);
    if (i == n - 1 || i == Code::kIndexCodeMaxLength) {
      return &page.entries;
    }
    else {
      if (!page.next_level) {
        page.next_level = New<Vocabulary>();
      }
      v = page.next_level.get();
    }
  }
  return NULL;
}

void Vocabulary::SortHomophones() {
  for (auto& v : *this) {
    auto& page(v.second);
    page.entries.Sort();
    if (page.next_level)
      page.next_level->SortHomophones();
  }
}

}  // namespace rime
