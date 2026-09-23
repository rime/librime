//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/dict/rewrite_pack.h>
#include "rewrite_internal.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <darts.h>

namespace rime {
namespace {

using rewrite_internal::FitsU32;
using rewrite_internal::FlushFileDurably;
using rewrite_internal::InRange;
using rewrite_internal::InstallAtomically;
using rewrite_internal::MakeTemporaryPath;
using rewrite_internal::ReadU32;
using rewrite_internal::ReadU64;
using rewrite_internal::TemporaryFileGuard;
using rewrite_internal::WriteU32;
using rewrite_internal::WriteU64;
using rewrite_internal::WriteZeros;

constexpr char kMagic[] = {'R', 'I', 'M', 'E', 'R', 'W', 'P', '1'};
constexpr uint32_t kFormatVersion = 1;
constexpr uint32_t kStageRecordOffset = 16;
constexpr uint32_t kStageRecordSize = 72;
constexpr uint32_t kHeaderSize = kStageRecordOffset + kStageRecordSize;
constexpr uint32_t kKeyIndexRecordSize = 8;
constexpr uint32_t kStageFlagHasPreedit = 1U << 0;
constexpr uint32_t kKnownStageFlags = kStageFlagHasPreedit;

constexpr uint32_t kUnicodeScalarLimit = 0x110000;
constexpr uint32_t kCharPageShift = 8;
constexpr uint32_t kCharPageSize = 1U << kCharPageShift;
constexpr uint32_t kCharPageMask = kCharPageSize - 1;
constexpr uint32_t kCharDirectoryEntries = kUnicodeScalarLimit / kCharPageSize;
constexpr uint32_t kDispatchPhraseStarter = 1U << 31;
constexpr uint32_t kDispatchKeyMask = kDispatchPhraseStarter - 1;
constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

static_assert(kCharDirectoryEntries <= std::numeric_limits<uint16_t>::max());
static_assert(sizeof(uint16_t) == 2 && sizeof(uint32_t) == 4);

struct StageRecord {
  uint64_t phrase_trie_offset = 0;
  uint64_t phrase_trie_size = 0;
  uint64_t char_directory_offset = 0;
  uint64_t char_pages_offset = 0;
  uint64_t key_index_offset = 0;
  uint64_t value_pool_offset = 0;
  uint64_t value_pool_size = 0;
  uint32_t char_page_count = 0;
  uint32_t key_count = 0;
  uint32_t flags = 0;
  uint32_t phrase_key_count = 0;
};

uint64_t Tell(std::ostream* out) {
  const std::streampos position = out->tellp();
  return position < 0 ? 0 : static_cast<uint64_t>(position);
}

void Align(std::ostream* out, size_t alignment) {
  const uint64_t position = Tell(out);
  WriteZeros(
      out, static_cast<size_t>((alignment - position % alignment) % alignment));
}

StageRecord ReadStageRecord(const char* p) {
  StageRecord record;
  record.phrase_trie_offset = ReadU64(p);
  record.phrase_trie_size = ReadU64(p + 8);
  record.char_directory_offset = ReadU64(p + 16);
  record.char_pages_offset = ReadU64(p + 24);
  record.key_index_offset = ReadU64(p + 32);
  record.value_pool_offset = ReadU64(p + 40);
  record.value_pool_size = ReadU64(p + 48);
  record.char_page_count = ReadU32(p + 56);
  record.key_count = ReadU32(p + 60);
  record.flags = ReadU32(p + 64);
  record.phrase_key_count = ReadU32(p + 68);
  return record;
}

void WriteStageRecord(std::ostream* out, const StageRecord& record) {
  WriteU64(out, record.phrase_trie_offset);
  WriteU64(out, record.phrase_trie_size);
  WriteU64(out, record.char_directory_offset);
  WriteU64(out, record.char_pages_offset);
  WriteU64(out, record.key_index_offset);
  WriteU64(out, record.value_pool_offset);
  WriteU64(out, record.value_pool_size);
  WriteU32(out, record.char_page_count);
  WriteU32(out, record.key_count);
  WriteU32(out, record.flags);
  WriteU32(out, record.phrase_key_count);
}

uint32_t DartsUnitLabel(uint32_t unit) {
  return unit & ((1U << 31) | 0xffU);
}

uint32_t DartsUnitOffset(uint32_t unit) {
  return (unit >> 10) << ((unit & (1U << 9)) >> 6);
}

bool ValidatePhraseTrieBounds(const char* data, uint64_t size) {
  constexpr uint64_t kDartsBlockUnits = 256;
  constexpr uint64_t kDartsUnitSize = sizeof(uint32_t);
  if (!data || size % kDartsUnitSize != 0) {
    return false;
  }
  const uint64_t unit_count = size / kDartsUnitSize;
  if (unit_count < kDartsBlockUnits || unit_count % kDartsBlockUnits != 0 ||
      unit_count > std::numeric_limits<uint32_t>::max()) {
    return false;
  }
  for (uint64_t i = 0; i < unit_count; ++i) {
    uint32_t unit = 0;
    std::memcpy(&unit, data + i * kDartsUnitSize, sizeof(unit));
    if (i != 0 && DartsUnitLabel(unit) > 0xffU) {
      continue;
    }
    if ((i ^ DartsUnitOffset(unit)) >= unit_count) {
      return false;
    }
  }
  return true;
}

bool ValidateStageRecord(const StageRecord& record, uint64_t file_size) {
  const uint64_t char_directory_size =
      static_cast<uint64_t>(kCharDirectoryEntries) * sizeof(uint16_t);
  const uint64_t char_pages_size =
      static_cast<uint64_t>(record.char_page_count) * kCharPageSize *
      sizeof(uint32_t);
  const bool has_phrase_trie = record.phrase_key_count != 0;
  return record.key_count != 0 && record.key_count <= kDispatchKeyMask &&
         record.char_page_count != 0 &&
         record.char_page_count <= kCharDirectoryEntries &&
         record.value_pool_size != 0 &&
         (record.flags & ~kKnownStageFlags) == 0 &&
         record.char_directory_offset >= kHeaderSize &&
         (record.char_directory_offset % alignof(uint16_t)) == 0 &&
         (record.char_pages_offset % alignof(uint32_t)) == 0 &&
         InRange(record.char_directory_offset, char_directory_size,
                 file_size) &&
         InRange(record.char_pages_offset, char_pages_size, file_size) &&
         InRange(record.key_index_offset,
                 static_cast<uint64_t>(record.key_count) * kKeyIndexRecordSize,
                 file_size) &&
         InRange(record.value_pool_offset, record.value_pool_size, file_size) &&
         ((!has_phrase_trie && record.phrase_trie_offset == 0 &&
           record.phrase_trie_size == 0) ||
          (has_phrase_trie && record.phrase_trie_size != 0 &&
           (record.phrase_trie_offset % alignof(uint32_t)) == 0 &&
           (record.phrase_trie_size % sizeof(uint32_t)) == 0 &&
           InRange(record.phrase_trie_offset, record.phrase_trie_size,
                   file_size)));
}

size_t DecodeUtf8Scalar(const char* begin,
                        const char* end,
                        uint32_t* code_point) {
  if (!begin || !end || !code_point || begin >= end) {
    return 0;
  }

  const auto* p = reinterpret_cast<const unsigned char*>(begin);
  const size_t remaining = static_cast<size_t>(end - begin);
  const unsigned char first = p[0];

  if (first <= 0x7f) {
    *code_point = first;
    return 1;
  }
  if (first >= 0xc2 && first <= 0xdf) {
    if (remaining < 2 || (p[1] & 0xc0) != 0x80) {
      return 0;
    }
    *code_point = (static_cast<uint32_t>(first & 0x1f) << 6) |
                  static_cast<uint32_t>(p[1] & 0x3f);
    return 2;
  }
  if (first >= 0xe0 && first <= 0xef) {
    if (remaining < 3 || (p[1] & 0xc0) != 0x80 || (p[2] & 0xc0) != 0x80) {
      return 0;
    }
    if ((first == 0xe0 && p[1] < 0xa0) || (first == 0xed && p[1] > 0x9f)) {
      return 0;
    }
    *code_point = (static_cast<uint32_t>(first & 0x0f) << 12) |
                  (static_cast<uint32_t>(p[1] & 0x3f) << 6) |
                  static_cast<uint32_t>(p[2] & 0x3f);
    return 3;
  }
  if (first >= 0xf0 && first <= 0xf4) {
    if (remaining < 4 || (p[1] & 0xc0) != 0x80 || (p[2] & 0xc0) != 0x80 ||
        (p[3] & 0xc0) != 0x80) {
      return 0;
    }
    if ((first == 0xf0 && p[1] < 0x90) || (first == 0xf4 && p[1] > 0x8f)) {
      return 0;
    }
    *code_point = (static_cast<uint32_t>(first & 0x07) << 18) |
                  (static_cast<uint32_t>(p[1] & 0x3f) << 12) |
                  (static_cast<uint32_t>(p[2] & 0x3f) << 6) |
                  static_cast<uint32_t>(p[3] & 0x3f);
    return 4;
  }
  return 0;
}

size_t EncodeUtf8Scalar(uint32_t code_point, char out[5]) {
  if (!out || code_point > 0x10ffff ||
      (code_point >= 0xd800 && code_point <= 0xdfff)) {
    return 0;
  }

  size_t size = 0;
  if (code_point <= 0x7f) {
    out[size++] = static_cast<char>(code_point);
  } else if (code_point <= 0x7ff) {
    out[size++] = static_cast<char>(0xc0 | (code_point >> 6));
    out[size++] = static_cast<char>(0x80 | (code_point & 0x3f));
  } else if (code_point <= 0xffff) {
    out[size++] = static_cast<char>(0xe0 | (code_point >> 12));
    out[size++] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3f));
    out[size++] = static_cast<char>(0x80 | (code_point & 0x3f));
  } else {
    out[size++] = static_cast<char>(0xf0 | (code_point >> 18));
    out[size++] = static_cast<char>(0x80 | ((code_point >> 12) & 0x3f));
    out[size++] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3f));
    out[size++] = static_cast<char>(0x80 | (code_point & 0x3f));
  }
  out[size] = '\0';
  return size;
}

bool ValidateUtf8Key(std::string_view key,
                     uint32_t* first_code_point,
                     size_t* first_char_size) {
  if (key.empty() || !first_code_point || !first_char_size) {
    return false;
  }

  const char* current = key.data();
  const char* end = current + key.size();
  *first_char_size = DecodeUtf8Scalar(current, end, first_code_point);
  if (*first_char_size == 0) {
    return false;
  }
  current += *first_char_size;
  while (current < end) {
    uint32_t code_point = 0;
    const size_t char_size = DecodeUtf8Scalar(current, end, &code_point);
    if (char_size == 0) {
      return false;
    }
    current += char_size;
  }
  return true;
}

size_t VarUint32Size(uint32_t value) {
  size_t size = 1;
  while (value >= 0x80) {
    value >>= 7;
    ++size;
  }
  return size;
}

void WriteVarUint32(std::ostream* out, uint32_t value) {
  char bytes[5];
  size_t size = 0;
  while (value >= 0x80) {
    bytes[size++] = static_cast<char>((value & 0x7f) | 0x80);
    value >>= 7;
  }
  bytes[size++] = static_cast<char>(value);
  out->write(bytes, size);
}

inline bool ReadVarUint32(const char* begin,
                          const char* end,
                          const char** next,
                          uint32_t* value) {
  if (!begin || !end || !next || !value || begin > end) {
    return false;
  }

  uint32_t result = 0;
  unsigned shift = 0;
  const char* p = begin;
  for (unsigned i = 0; i < 5 && p < end; ++i, ++p) {
    const uint8_t byte = static_cast<uint8_t>(*p);
    if (i == 4 && (byte & 0xf0) != 0) {
      return false;
    }
    result |= static_cast<uint32_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      *next = p + 1;
      *value = result;
      return true;
    }
    shift += 7;
  }
  return false;
}

class RewriteStageStorage {
 public:
  RewriteStageStorage() = default;
  ~RewriteStageStorage() { Close(); }

  bool Open(const path& file_path, uint64_t offset, uint64_t size) {
    Close();
    if (size == 0 || size > std::numeric_limits<size_t>::max()) {
      return false;
    }
#ifdef _WIN32
    HANDLE file =
        CreateFileW(file_path.wstring().c_str(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
      return false;
    }
    SYSTEM_INFO system_info{};
    GetSystemInfo(&system_info);
    const uint64_t granularity = system_info.dwAllocationGranularity;
    const uint64_t aligned_offset = offset / granularity * granularity;
    const uint64_t delta = offset - aligned_offset;
    if (delta > std::numeric_limits<size_t>::max() ||
        size > std::numeric_limits<size_t>::max() - delta) {
      CloseHandle(file);
      return false;
    }
    const size_t mapping_size = static_cast<size_t>(delta + size);
    HANDLE mapping =
        CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) {
      CloseHandle(file);
      return false;
    }
    void* base = MapViewOfFile(
        mapping, FILE_MAP_READ, static_cast<DWORD>(aligned_offset >> 32),
        static_cast<DWORD>(aligned_offset & 0xffffffff), mapping_size);
    CloseHandle(mapping);
    CloseHandle(file);
    if (!base) {
      return false;
    }
    mapped_base_ = base;
    mapped_size_ = mapping_size;
    data_ = static_cast<const char*>(base) + delta;
    return true;
#else
    try {
      mapping_ = std::make_unique<boost::interprocess::file_mapping>(
          file_path.c_str(), boost::interprocess::read_only);
      region_ = std::make_unique<boost::interprocess::mapped_region>(
          *mapping_, boost::interprocess::read_only,
          static_cast<boost::interprocess::offset_t>(offset),
          static_cast<size_t>(size));
      return region_ && region_->get_address() && region_->get_size() == size;
    } catch (const std::exception&) {
      mapping_.reset();
      region_.reset();
      return false;
    }
#endif
  }

  const char* data() const {
#ifdef _WIN32
    return data_;
#else
    return region_ ? static_cast<const char*>(region_->get_address()) : nullptr;
#endif
  }

 private:
  void Close() {
#ifdef _WIN32
    if (mapped_base_) {
      UnmapViewOfFile(mapped_base_);
    }
    mapped_base_ = nullptr;
    mapped_size_ = 0;
    data_ = nullptr;
#else
    region_.reset();
    mapping_.reset();
#endif
  }

#ifdef _WIN32
  void* mapped_base_ = nullptr;
  size_t mapped_size_ = 0;
  const char* data_ = nullptr;
#else
  std::unique_ptr<boost::interprocess::file_mapping> mapping_;
  std::unique_ptr<boost::interprocess::mapped_region> region_;
#endif
};

struct StageView {
  Darts::DoubleArray phrase_trie;
  const uint16_t* char_directory = nullptr;
  const uint32_t* char_pages = nullptr;
  const char* key_index = nullptr;
  const char* value_pool = nullptr;
  const char* value_pool_end = nullptr;
  uint32_t char_page_count = 0;
  uint32_t key_count = 0;
  uint32_t phrase_key_count = 0;
  uint32_t flags = 0;
  vector<uint32_t> phrase_nodes;

  size_t DispatchSlot(uint32_t code_point) const {
    if (!char_directory || !char_pages || code_point >= kUnicodeScalarLimit) {
      return kInvalidIndex;
    }
    const uint16_t page_id = char_directory[code_point >> kCharPageShift];
    if (page_id == 0 || page_id > char_page_count) {
      return kInvalidIndex;
    }
    return static_cast<size_t>(page_id - 1) * kCharPageSize +
           (code_point & kCharPageMask);
  }

  uint32_t Dispatch(uint32_t code_point) const {
    const size_t slot = DispatchSlot(code_point);
    return slot == kInvalidIndex ? 0 : char_pages[slot];
  }

  uint32_t DispatchAt(const char* current,
                      const char* end,
                      size_t* char_size,
                      size_t* dispatch_slot = nullptr) const {
    if (!char_size) {
      return 0;
    }
    if (dispatch_slot) {
      *dispatch_slot = kInvalidIndex;
    }

    uint32_t code_point = 0;
    *char_size = DecodeUtf8Scalar(current, end, &code_point);
    if (*char_size == 0) {
      *char_size = 1;
      return 0;
    }

    const size_t slot = DispatchSlot(code_point);
    if (dispatch_slot) {
      *dispatch_slot = slot;
    }
    return slot == kInvalidIndex ? 0 : char_pages[slot];
  }

  bool BuildPhraseNodeCache() {
    phrase_nodes.clear();
    if (phrase_key_count == 0) {
      return true;
    }
    if (!char_directory || !char_pages) {
      return false;
    }

    phrase_nodes.assign(static_cast<size_t>(char_page_count) * kCharPageSize,
                        0);

    for (uint32_t high = 0; high < kCharDirectoryEntries; ++high) {
      const uint16_t page_id = char_directory[high];
      if (page_id == 0) {
        continue;
      }
      if (page_id > char_page_count) {
        return false;
      }

      const size_t page_offset =
          static_cast<size_t>(page_id - 1) * kCharPageSize;
      for (uint32_t low = 0; low < kCharPageSize; ++low) {
        const size_t slot = page_offset + low;
        if ((char_pages[slot] & kDispatchPhraseStarter) == 0) {
          continue;
        }

        const uint32_t code_point = (high << kCharPageShift) | low;
        char encoded[5] = {};
        const size_t encoded_size = EncodeUtf8Scalar(code_point, encoded);
        if (encoded_size == 0) {
          return false;
        }

        size_t node_pos = 0;
        size_t key_pos = 0;
        phrase_trie.traverse(encoded, node_pos, key_pos);
        if (key_pos != encoded_size ||
            node_pos >= std::numeric_limits<uint32_t>::max()) {
          return false;
        }

        // Zero means no phrase continuation.
        phrase_nodes[slot] = static_cast<uint32_t>(node_pos + 1);
      }
    }
    return true;
  }

  bool PhraseNode(size_t dispatch_slot, size_t* node_pos) const {
    if (!node_pos || dispatch_slot == kInvalidIndex ||
        dispatch_slot >= phrase_nodes.size()) {
      return false;
    }
    const uint32_t encoded = phrase_nodes[dispatch_slot];
    if (encoded == 0) {
      return false;
    }
    *node_pos = static_cast<size_t>(encoded - 1);
    return true;
  }

  bool FindLongestPhrase(const char* suffix,
                         const char* end,
                         size_t node_pos,
                         Darts::DoubleArray::result_pair_type* longest) const {
    if (!suffix || !end || !longest || suffix >= end || phrase_key_count == 0) {
      return false;
    }

    *longest = {};
    const size_t remaining = static_cast<size_t>(end - suffix);
    const size_t max_possible_results =
        std::min(remaining, static_cast<size_t>(phrase_key_count));
    if (max_possible_results == 0) {
      return false;
    }

    constexpr size_t kInlinePrefixResults = 8;
    std::array<Darts::DoubleArray::result_pair_type, kInlinePrefixResults>
        inline_results{};

    auto select_longest = [&](const auto* results, size_t count) {
      for (size_t i = 0; i < count; ++i) {
        if (results[i].length > longest->length) {
          *longest = results[i];
        }
      }
    };

    size_t capacity = inline_results.size();
    size_t match_count = phrase_trie.commonPrefixSearch(
        suffix, inline_results.data(), capacity, remaining, node_pos);
    select_longest(inline_results.data(), std::min(match_count, capacity));

    if (match_count < capacity || capacity >= max_possible_results) {
      return longest->value > 0;
    }

    vector<Darts::DoubleArray::result_pair_type> results;
    while (capacity < max_possible_results) {
      size_t next_capacity = capacity * 2;
      if (match_count > next_capacity) {
        next_capacity = match_count;
      }
      capacity = std::min(next_capacity, max_possible_results);
      results.resize(capacity);

      match_count = phrase_trie.commonPrefixSearch(
          suffix, results.data(), capacity, remaining, node_pos);
      *longest = {};
      select_longest(results.data(), std::min(match_count, capacity));

      if (match_count < capacity || capacity >= max_possible_results) {
        return longest->value > 0;
      }
    }
    return longest->value > 0;
  }

  static bool ReadValue(const char** cursor,
                        const char* end,
                        std::string_view* value) {
    if (!cursor || !*cursor || !end || !value || *cursor > end) {
      return false;
    }
    uint32_t size = 0;
    if (!ReadVarUint32(*cursor, end, cursor, &size) ||
        static_cast<size_t>(end - *cursor) < size) {
      return false;
    }
    *value = std::string_view(*cursor, size);
    *cursor += size;
    return true;
  }

  bool ReadStoredValue(const char** cursor,
                       const char* end,
                       std::string_view* preedit,
                       std::string_view* value) const {
    if (!preedit || !value) {
      return false;
    }
    *preedit = std::string_view();
    if ((flags & kStageFlagHasPreedit) != 0 &&
        !ReadValue(cursor, end, preedit)) {
      return false;
    }
    return ReadValue(cursor, end, value);
  }

  bool GetValueList(size_t key_id,
                    const char** begin,
                    const char** end,
                    uint32_t* value_count) const {
    if (!begin || !end || !value_count || !key_index || !value_pool ||
        !value_pool_end || key_id >= key_count) {
      return false;
    }

    const char* index = key_index + key_id * kKeyIndexRecordSize;
    const uint32_t value_list_offset = ReadU32(index);
    const uint32_t count = ReadU32(index + 4);
    const size_t pool_size = static_cast<size_t>(value_pool_end - value_pool);
    if (count == 0 || value_list_offset >= pool_size) {
      return false;
    }

    *begin = value_pool + value_list_offset;
    *end = value_pool_end;
    *value_count = count;
    return true;
  }

  bool FindExactKey(std::string_view text, size_t* key_id) const {
    if (!key_id || text.empty()) {
      return false;
    }

    uint32_t code_point = 0;
    const size_t first_size =
        DecodeUtf8Scalar(text.data(), text.data() + text.size(), &code_point);
    if (first_size == 0) {
      return false;
    }

    const uint32_t dispatch = Dispatch(code_point);
    if (first_size == text.size()) {
      const uint32_t encoded_key_id = dispatch & kDispatchKeyMask;
      if (encoded_key_id == 0) {
        return false;
      }
      *key_id = static_cast<size_t>(encoded_key_id - 1);
      return *key_id < key_count;
    }

    if ((dispatch & kDispatchPhraseStarter) == 0 || phrase_key_count == 0) {
      return false;
    }

    Darts::DoubleArray::result_pair_type match{};
    phrase_trie.exactMatchSearch(text.data(), match, text.size());
    if (match.value <= 0 || match.length != text.size()) {
      return false;
    }
    *key_id = static_cast<size_t>(match.value - 1);
    return *key_id < key_count;
  }

  bool Lookup(std::string_view text, vector<string>* values) const {
    if (!values) {
      return false;
    }
    values->clear();

    size_t key_id = 0;
    if (!FindExactKey(text, &key_id)) {
      return false;
    }

    const char* cursor = nullptr;
    const char* end = nullptr;
    uint32_t value_count = 0;
    if (!GetValueList(key_id, &cursor, &end, &value_count)) {
      return false;
    }

    values->reserve(std::min<uint32_t>(value_count, 16));
    for (uint32_t i = 0; i < value_count; ++i) {
      std::string_view preedit;
      std::string_view value;
      if (!ReadStoredValue(&cursor, end, &preedit, &value)) {
        values->clear();
        return false;
      }
      values->emplace_back(value);
    }
    return true;
  }

  bool LookupWithPreedit(std::string_view text,
                         vector<RewriteResult>* values) const {
    if (!values) {
      return false;
    }
    values->clear();

    size_t key_id = 0;
    if (!FindExactKey(text, &key_id)) {
      return false;
    }

    const char* cursor = nullptr;
    const char* end = nullptr;
    uint32_t value_count = 0;
    if (!GetValueList(key_id, &cursor, &end, &value_count)) {
      return false;
    }

    values->reserve(std::min<uint32_t>(value_count, 16));
    for (uint32_t i = 0; i < value_count; ++i) {
      std::string_view preedit;
      std::string_view value;
      if (!ReadStoredValue(&cursor, end, &preedit, &value)) {
        values->clear();
        return false;
      }
      values->push_back({string(value.data(), value.size()),
                         string(preedit.data(), preedit.size())});
    }
    return true;
  }

  bool GetFirstValue(size_t key_id, std::string_view* value) const {
    const char* cursor = nullptr;
    const char* end = nullptr;
    uint32_t value_count = 0;
    if (!GetValueList(key_id, &cursor, &end, &value_count)) {
      return false;
    }

    std::string_view preedit;
    return ReadStoredValue(&cursor, end, &preedit, value);
  }

  bool ConvertSentence(std::string_view text, string* result) const {
    result->clear();

    bool changed = false;
    size_t pos = 0;
    const char* const text_begin = text.data();
    const char* const text_end = text_begin + text.size();

    while (pos < text.size()) {
      const char* current = text_begin + pos;
      size_t char_size = 0;
      size_t dispatch_slot = kInvalidIndex;
      uint32_t dispatch =
          DispatchAt(current, text_end, &char_size, &dispatch_slot);

      if (dispatch == 0) {
        const char* scan = current + char_size;
        while (scan < text_end) {
          size_t scan_size = 0;
          if (DispatchAt(scan, text_end, &scan_size) != 0) {
            break;
          }
          scan += scan_size;
        }
        const size_t skipped = static_cast<size_t>(scan - current);
        if (changed) {
          result->append(current, skipped);
        }
        pos += skipped;
        continue;
      }

      size_t key_id = kInvalidIndex;
      size_t match_size = 0;

      if ((dispatch & kDispatchPhraseStarter) != 0 && phrase_key_count != 0) {
        size_t node_pos = 0;
        Darts::DoubleArray::result_pair_type match{};
        if (PhraseNode(dispatch_slot, &node_pos) &&
            FindLongestPhrase(current + char_size, text_end, node_pos,
                              &match) &&
            match.length != 0) {
          const size_t candidate_key_id = static_cast<size_t>(match.value - 1);
          if (candidate_key_id < key_count) {
            key_id = candidate_key_id;
            match_size = char_size + match.length;
          }
        }
      }

      if (key_id == kInvalidIndex) {
        const uint32_t encoded_key_id = dispatch & kDispatchKeyMask;
        if (encoded_key_id != 0) {
          const size_t candidate_key_id =
              static_cast<size_t>(encoded_key_id - 1);
          if (candidate_key_id < key_count) {
            key_id = candidate_key_id;
            match_size = char_size;
          }
        }
      }

      if (key_id != kInvalidIndex) {
        std::string_view value;
        if (GetFirstValue(key_id, &value)) {
          const bool replacement_changed =
              value.size() != match_size ||
              std::memcmp(value.data(), current, match_size) != 0;
          if (replacement_changed) {
            if (!changed) {
              result->reserve(text.size());
              result->assign(text.data(), pos);
            }
            changed = true;
          }
          if (changed && !replacement_changed) {
            result->append(current, match_size);
          } else if (changed) {
            result->append(value.data(), value.size());
          }
          pos += match_size;
          continue;
        }
      }

      if (changed) {
        result->append(current, char_size);
      }
      pos += char_size;
    }
    return changed;
  }
};

struct StoredValue {
  std::string_view value;
  std::string_view preedit;
};

struct Bucket {
  std::string_view key;
  vector<StoredValue> values;
};

bool BuildBuckets(const RewriteStageData& stage, vector<Bucket>* buckets) {
  if (!buckets) {
    return false;
  }
  buckets->clear();
  buckets->reserve(stage.entries.size());
  std::unordered_map<std::string_view, size_t> key_to_bucket;
  key_to_bucket.reserve(stage.entries.size());

  for (const auto& entry : stage.entries) {
    if (entry.key.empty() || entry.value.empty()) {
      continue;
    }
    const std::string_view key(entry.key.data(), entry.key.size());
    const std::string_view value(entry.value.data(), entry.value.size());
    const std::string_view preedit(entry.preedit.data(), entry.preedit.size());
    auto [it, inserted] = key_to_bucket.emplace(key, buckets->size());
    if (inserted) {
      buckets->push_back(Bucket{key, {}});
    }
    auto& values = (*buckets)[it->second].values;
    if (std::none_of(values.begin(), values.end(),
                     [&](const auto& item) { return item.value == value; })) {
      values.push_back({value, preedit});
    }
  }
  return !buckets->empty() && FitsU32(buckets->size()) &&
         buckets->size() <= kDispatchKeyMask;
}

bool WriteDispatchAndTrie(std::ostream* out,
                          const vector<Bucket>& buckets,
                          StageRecord* record) {
  std::array<uint16_t, kCharDirectoryEntries> char_directory{};
  vector<std::array<uint32_t, kCharPageSize>> char_pages;
  vector<size_t> phrase_ids;
  phrase_ids.reserve(buckets.size());

  auto page_for =
      [&](uint32_t code_point) -> std::array<uint32_t, kCharPageSize>& {
    uint16_t& page_id = char_directory[code_point >> kCharPageShift];
    if (page_id == 0) {
      char_pages.emplace_back();
      page_id = static_cast<uint16_t>(char_pages.size());
    }
    return char_pages[page_id - 1];
  };

  for (size_t key_id = 0; key_id < buckets.size(); ++key_id) {
    const auto& bucket = buckets[key_id];
    if (bucket.key.find('\0') != std::string_view::npos) {
      LOG(ERROR) << "rewrite key contains NUL bytes.";
      return false;
    }
    uint32_t first_code_point = 0;
    size_t first_char_size = 0;
    if (!ValidateUtf8Key(bucket.key, &first_code_point, &first_char_size)) {
      LOG(ERROR) << "rewrite key contains invalid UTF-8.";
      return false;
    }

    uint32_t& dispatch =
        page_for(first_code_point)[first_code_point & kCharPageMask];
    const uint32_t encoded_key_id = static_cast<uint32_t>(key_id + 1);
    if (first_char_size == bucket.key.size()) {
      if ((dispatch & kDispatchKeyMask) != 0) {
        return false;
      }
      dispatch |= encoded_key_id;
    } else {
      dispatch |= kDispatchPhraseStarter;
      phrase_ids.push_back(key_id);
    }
  }

  if (char_pages.empty()) {
    return false;
  }
  Align(out, alignof(uint32_t));
  record->char_directory_offset = Tell(out);
  out->write(reinterpret_cast<const char*>(char_directory.data()),
             static_cast<std::streamsize>(sizeof(char_directory)));
  record->char_pages_offset = Tell(out);
  for (const auto& page : char_pages) {
    out->write(reinterpret_cast<const char*>(page.data()),
               static_cast<std::streamsize>(sizeof(page)));
  }
  record->char_page_count = static_cast<uint32_t>(char_pages.size());
  if (!*out) {
    return false;
  }

  const auto byte_less = [&](size_t lhs_id, size_t rhs_id) {
    const auto lhs = buckets[lhs_id].key;
    const auto rhs = buckets[rhs_id].key;
    return std::lexicographical_compare(
        lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](char a, char b) {
          return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
        });
  };
  std::sort(phrase_ids.begin(), phrase_ids.end(), byte_less);
  record->phrase_key_count = static_cast<uint32_t>(phrase_ids.size());
  if (phrase_ids.empty()) {
    return true;
  }

  vector<const char*> keys;
  vector<size_t> lengths;
  vector<int> values;
  keys.reserve(phrase_ids.size());
  lengths.reserve(phrase_ids.size());
  values.reserve(phrase_ids.size());
  for (const size_t key_id : phrase_ids) {
    keys.push_back(buckets[key_id].key.data());
    lengths.push_back(buckets[key_id].key.size());
    values.push_back(static_cast<int>(key_id + 1));
  }

  Darts::DoubleArray trie;
  try {
    trie.build(keys.size(), keys.data(), lengths.data(), values.data());
  } catch (const std::exception& ex) {
    LOG(ERROR) << "cannot build rewrite phrase trie: " << ex.what();
    return false;
  } catch (...) {
    LOG(ERROR) << "cannot build rewrite phrase trie.";
    return false;
  }
  if (trie.total_size() == 0 || (trie.total_size() % trie.unit_size()) != 0) {
    return false;
  }

  Align(out, alignof(uint32_t));
  record->phrase_trie_offset = Tell(out);
  record->phrase_trie_size = trie.total_size();
  out->write(reinterpret_cast<const char*>(trie.array()),
             static_cast<std::streamsize>(trie.total_size()));
  return static_cast<bool>(*out);
}

bool WriteValues(std::ostream* out,
                 const vector<Bucket>& buckets,
                 bool has_preedit,
                 StageRecord* record) {
  record->key_index_offset = Tell(out);
  record->key_count = static_cast<uint32_t>(buckets.size());
  uint64_t value_pool_size = 0;

  for (const auto& bucket : buckets) {
    if (bucket.values.empty() || !FitsU32(bucket.values.size()) ||
        value_pool_size > std::numeric_limits<uint32_t>::max()) {
      return false;
    }
    WriteU32(out, static_cast<uint32_t>(value_pool_size));
    WriteU32(out, static_cast<uint32_t>(bucket.values.size()));
    for (const auto& item : bucket.values) {
      if (!FitsU32(item.preedit.size()) || !FitsU32(item.value.size())) {
        return false;
      }
      uint64_t encoded =
          VarUint32Size(static_cast<uint32_t>(item.value.size())) +
          item.value.size();
      if (has_preedit) {
        encoded += VarUint32Size(static_cast<uint32_t>(item.preedit.size())) +
                   item.preedit.size();
      }
      if (encoded > std::numeric_limits<uint32_t>::max() - value_pool_size) {
        return false;
      }
      value_pool_size += encoded;
    }
  }

  record->value_pool_offset = Tell(out);
  record->value_pool_size = value_pool_size;
  for (const auto& bucket : buckets) {
    for (const auto& item : bucket.values) {
      if (has_preedit) {
        WriteVarUint32(out, static_cast<uint32_t>(item.preedit.size()));
        out->write(item.preedit.data(), item.preedit.size());
      }
      WriteVarUint32(out, static_cast<uint32_t>(item.value.size()));
      out->write(item.value.data(), item.value.size());
    }
  }
  return *out && Tell(out) == record->value_pool_offset + value_pool_size;
}

bool BuildRewritePack(const path& output_path, const RewriteStageData& stage) {
  if (stage.entries.empty()) {
    return false;
  }
  std::error_code ec;
  const path parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      return false;
    }
  }

  const path temporary_path = MakeTemporaryPath(output_path);
  TemporaryFileGuard temporary_file(temporary_path);
  std::ofstream out(static_cast<const std::filesystem::path&>(temporary_path),
                    std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }

  out.write(kMagic, sizeof(kMagic));
  WriteU32(&out, kFormatVersion);
  WriteU32(&out, kHeaderSize);
  WriteZeros(&out, kStageRecordSize);

  StageRecord record;
  const bool has_preedit =
      std::any_of(stage.entries.begin(), stage.entries.end(),
                  [](const auto& entry) { return !entry.preedit.empty(); });
  if (has_preedit) {
    record.flags |= kStageFlagHasPreedit;
  }

  vector<Bucket> buckets;
  if (!BuildBuckets(stage, &buckets) ||
      !WriteDispatchAndTrie(&out, buckets, &record) ||
      !WriteValues(&out, buckets, has_preedit, &record)) {
    return false;
  }

  const uint64_t end = Tell(&out);
  out.seekp(kStageRecordOffset);
  WriteStageRecord(&out, record);
  out.seekp(static_cast<std::streamoff>(end));
  out.close();
  if (!out || !FlushFileDurably(temporary_path) ||
      !InstallAtomically(temporary_path, output_path, &ec)) {
    return false;
  }
  temporary_file.Commit();
  return true;
}

}  // namespace

class RewritePack::Impl {
 public:
  std::unique_ptr<RewriteStageStorage> storage;
  StageView stage;
};

RewritePack::RewritePack(const path& file_path, uint64_t offset, uint64_t size)
    : file_path_(file_path), offset_(offset), size_(size), impl_(new Impl) {}

RewritePack::~RewritePack() = default;

bool RewritePack::Open() {
  impl_->storage = std::make_unique<RewriteStageStorage>();
  if (!impl_->storage->Open(file_path_, offset_, size_)) {
    return false;
  }

  const char* base = impl_->storage->data();
  if (!base || size_ < kHeaderSize ||
      std::memcmp(base, kMagic, sizeof(kMagic)) != 0 ||
      ReadU32(base + 8) != kFormatVersion ||
      ReadU32(base + 12) != kHeaderSize) {
    return false;
  }

  const StageRecord record = ReadStageRecord(base + kStageRecordOffset);
  if (!ValidateStageRecord(record, size_)) {
    return false;
  }

  StageView& stage = impl_->stage;
  stage.char_directory =
      reinterpret_cast<const uint16_t*>(base + record.char_directory_offset);
  stage.char_pages =
      reinterpret_cast<const uint32_t*>(base + record.char_pages_offset);
  stage.char_page_count = record.char_page_count;
  stage.key_index = base + record.key_index_offset;
  stage.value_pool = base + record.value_pool_offset;
  stage.value_pool_end = stage.value_pool + record.value_pool_size;
  stage.key_count = record.key_count;
  stage.phrase_key_count = record.phrase_key_count;
  stage.flags = record.flags;

  if (record.phrase_key_count != 0) {
    const char* phrase_trie = base + record.phrase_trie_offset;
    if (!ValidatePhraseTrieBounds(phrase_trie, record.phrase_trie_size)) {
      return false;
    }
    stage.phrase_trie.set_array(
        phrase_trie, static_cast<size_t>(record.phrase_trie_size /
                                         stage.phrase_trie.unit_size()));
    if (!stage.BuildPhraseNodeCache()) {
      return false;
    }
  }
  return true;
}

bool RewritePack::Lookup(std::string_view text, vector<string>* values) const {
  return impl_->stage.Lookup(text, values);
}

bool RewritePack::LookupWithPreedit(std::string_view text,
                                    vector<RewriteResult>* values) const {
  return impl_->stage.LookupWithPreedit(text, values);
}

bool RewritePack::ConvertSentence(std::string_view text, string* result) const {
  return result && impl_->stage.ConvertSentence(text, result);
}

bool RewritePackBuilder::Build(const path& output_path,
                               const RewriteStageData& stage) const {
  return BuildRewritePack(output_path, stage);
}

}  // namespace rime
