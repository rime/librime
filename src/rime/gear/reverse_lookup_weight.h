//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Order reverse-lookup annotation codes using weights already stored in
// deployed .table.bin files. No reverse DB format or rebuild is involved.
//
#ifndef RIME_REVERSE_LOOKUP_WEIGHT_H_
#define RIME_REVERSE_LOOKUP_WEIGHT_H_

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <rime/dict/dictionary.h>

namespace rime {

inline void SortReverseLookupCodesByWeight(const string& text,
                                           string* codes,
                                           Dictionary* dictionary) {
  if (!codes || !dictionary || codes->find(' ') == string::npos)
    return;

  struct Reading {
    string code;
    double weight = 0.0;
    bool found = false;
  };

  vector<Reading> readings;
  std::istringstream input(*codes);
  for (string code; input >> code;)
    readings.push_back({code});
  if (readings.size() < 2)
    return;

  bool any_weight = false;
  // A pack may have its own syllabary and syllable IDs, so resolve each
  // reading separately against each loaded table, not via the main prism.
  for (const auto& table_ptr : dictionary->tables()) {
    Table* table = table_ptr.get();
    if (!table || !table->IsOpen() || !table->metadata())
      continue;
    const int syllable_count =
        static_cast<int>(table->metadata()->num_syllables);
    for (auto& reading : readings) {
      // The compiled syllabary is sorted; binary-search its existing IDs.
      int first = 0;
      int last = syllable_count;
      while (first < last) {
        const int middle = first + (last - first) / 2;
        if (table->GetSyllableById(middle) < reading.code)
          first = middle + 1;
        else
          last = middle;
      }
      if (first == syllable_count ||
          table->GetSyllableById(first) != reading.code)
        continue;

      // Match the actual character/word, never the weight of another
      // homophone. Across base and packs, retain its highest weight.
      auto entries = table->QueryWords(first);
      while (!entries.exhausted()) {
        const auto* entry = entries.entry();
        if (entry && table->GetEntryText(*entry) == text) {
          if (!reading.found || entry->weight > reading.weight)
            reading.weight = entry->weight;
          reading.found = true;
          any_weight = true;
        }
        entries.Next();
      }
    }
  }
  if (!any_weight)
    return;  // Leave unresolvable annotations exactly as they were.

  std::stable_sort(readings.begin(), readings.end(),
                   [](const Reading& a, const Reading& b) {
                     if (a.found != b.found)
                       return a.found;
                     return a.found && a.weight > b.weight;
                   });
  string sorted;
  sorted.reserve(codes->size());
  for (const auto& reading : readings) {
    if (!sorted.empty())
      sorted += ' ';
    sorted += reading.code;
  }
  *codes = std::move(sorted);
}

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_WEIGHT_H_
