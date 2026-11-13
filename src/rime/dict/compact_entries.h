//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Temporary compact data used while compiling single-syllable dictionaries.
//
#ifndef RIME_COMPACT_ENTRIES_H_
#define RIME_COMPACT_ENTRIES_H_

#include <cstdint>
#include <string_view>
#include <rime/common.h>
#include <rime/dict/vocabulary.h>

namespace rime {

// Build-time representation for a single-syllable dictionary entry. This is
// intentionally separate from ShortDictEntry, which also represents regular
// multi-syllable Vocabulary pages.
struct CompactDictEntry {
  size_t text_offset = 0;
  uint32_t text_length = 0;
  SyllableId syllable_id = 0;
  double weight = 0.0;
};

struct CompactDictEntryList {
  string text;
  vector<CompactDictEntry> entries;
  vector<size_t> page_offsets;

  std::string_view text_of(const CompactDictEntry& entry) const {
    return std::string_view(text.data() + entry.text_offset, entry.text_length);
  }
};

}  // namespace rime

#endif  // RIME_COMPACT_ENTRIES_H_
