//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/dict/rewrite_pack.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <mutex>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <marisa.h>
#include <utf8.h>

#include <rime/dict/mapped_file.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace rime {
namespace {

constexpr char kMagic[] = {'R', 'I', 'M', 'E', 'R', 'W', 'P', '2'};
constexpr uint32_t kFormatVersion = 2;
constexpr uint32_t kHeaderSize = 40;
constexpr uint32_t kSectionRecordSize = 24;
constexpr uint32_t kStageRecordSize = 64;
constexpr uint32_t kKeyIndexRecordSize = 8;
constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

struct SectionRecord {
  uint64_t name_offset = 0;
  uint32_t name_size = 0;
  uint32_t stage_count = 0;
  uint64_t stage_table_offset = 0;
};

struct StageRecord {
  uint64_t trie_offset = 0;
  uint64_t trie_size = 0;
  uint64_t key_index_offset = 0;
  uint32_t key_count = 0;
  uint64_t value_pool_offset = 0;
  uint64_t value_pool_size = 0;
};

inline uint32_t ReadU32(const char* p) {
  const auto* b = reinterpret_cast<const unsigned char*>(p);
  return static_cast<uint32_t>(b[0]) |
         (static_cast<uint32_t>(b[1]) << 8) |
         (static_cast<uint32_t>(b[2]) << 16) |
         (static_cast<uint32_t>(b[3]) << 24);
}

inline uint64_t ReadU64(const char* p) {
  uint64_t value = 0;
  for (int i = 7; i >= 0; --i) {
    value = (value << 8) |
            static_cast<uint64_t>(static_cast<unsigned char>(p[i]));
  }
  return value;
}

void WriteU32(std::ostream* out, uint32_t value) {
  std::array<char, 4> bytes{};
  for (size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<char>((value >> (i * 8)) & 0xff);
  }
  out->write(bytes.data(), bytes.size());
}

void WriteU64(std::ostream* out, uint64_t value) {
  std::array<char, 8> bytes{};
  for (size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<char>((value >> (i * 8)) & 0xff);
  }
  out->write(bytes.data(), bytes.size());
}

void WriteZeros(std::ostream* out, size_t count) {
  static constexpr std::array<char, 64> kZeros{};
  while (count > 0) {
    const size_t chunk = std::min(count, kZeros.size());
    out->write(kZeros.data(), chunk);
    count -= chunk;
  }
}

uint64_t Tell(std::ostream* out) {
  const std::streampos position = out->tellp();
  return position < 0 ? 0 : static_cast<uint64_t>(position);
}

void Align(std::ostream* out, size_t alignment) {
  const uint64_t position = Tell(out);
  const size_t padding =
      static_cast<size_t>((alignment - position % alignment) % alignment);
  WriteZeros(out, padding);
}

bool InRange(uint64_t offset, uint64_t size, uint64_t file_size) {
  return offset <= file_size && size <= file_size - offset;
}

bool FitsU32(size_t value) {
  return value <= std::numeric_limits<uint32_t>::max();
}

size_t NextUtf8CharSize(const char* begin, const char* end) {
  if (!begin || !end || begin >= end) {
    return 0;
  }
  const char* next = begin;
  try {
    utf8::next(next, end);
  } catch (const std::exception&) {
    // Preserve malformed input byte-for-byte rather than reading past the
    // candidate buffer. Normal Rime text takes the fast, non-throwing path.
    return 1;
  }
  if (next <= begin || next > end) {
    return 1;
  }
  return static_cast<size_t>(next - begin);
}

std::atomic<uint64_t> g_temp_counter{0};

path MakeTemporaryPath(const path& output_path) {
  const uint64_t now = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const uint64_t thread = static_cast<uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
  const uint64_t sequence = g_temp_counter.fetch_add(1);

  path temporary_path = output_path;
  const string suffix = ".tmp." + std::to_string(now) + "." +
                        std::to_string(thread) + "." +
                        std::to_string(sequence);
  temporary_path += path(suffix);
  return temporary_path;
}

class TemporaryFileGuard {
 public:
  explicit TemporaryFileGuard(path file_path) : file_path_(std::move(file_path)) {}
  ~TemporaryFileGuard() {
    if (!committed_) {
      std::error_code ec;
      std::filesystem::remove(file_path_, ec);
    }
  }

  void Commit() { committed_ = true; }

 private:
  path file_path_;
  bool committed_ = false;
};

bool InstallAtomically(const path& temporary_path,
                       const path& output_path,
                       std::error_code* error) {
  if (!error) {
    return false;
  }
  error->clear();
  std::filesystem::rename(temporary_path, output_path, *error);
  if (!*error) {
    return true;
  }

#ifdef _WIN32
  // std::filesystem::rename is not required to replace an existing file on
  // Windows. MoveFileEx does the replacement without deleting the old pack
  // first, so a failed install leaves the previous valid pack intact.
  if (MoveFileExW(temporary_path.wstring().c_str(), output_path.wstring().c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    error->clear();
    return true;
  }
  *error = std::error_code(static_cast<int>(GetLastError()),
                           std::system_category());
#endif
  return false;
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

uint64_t HashBytes(uint64_t hash, const char* data, size_t size) {
  constexpr uint64_t kPrime = 1099511628211ULL;
  for (size_t i = 0; i < size; ++i) {
    hash ^= static_cast<unsigned char>(data[i]);
    hash *= kPrime;
  }
  return hash;
}

uint64_t HashU64(uint64_t hash, uint64_t value) {
  std::array<char, 8> bytes{};
  for (size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<char>((value >> (i * 8)) & 0xff);
  }
  return HashBytes(hash, bytes.data(), bytes.size());
}

uint64_t HashString(uint64_t hash, const string& value) {
  hash = HashU64(hash, value.size());
  return HashBytes(hash, value.data(), value.size());
}

uint64_t ComputeBuildId(const vector<RewriteSectionData>& sections) {
  uint64_t hash = 1469598103934665603ULL;
  hash = HashU64(hash, kFormatVersion);
  hash = HashU64(hash, sections.size());
  for (const auto& section : sections) {
    hash = HashString(hash, section.name);
    hash = HashU64(hash, section.stage.entries.size());
    for (const auto& entry : section.stage.entries) {
      hash = HashString(hash, entry.key);
      hash = HashString(hash, entry.value);
    }
  }
  return hash;
}

bool ReadPackBuildId(const path& file_path, uint64_t* build_id) {
  if (!build_id) {
    return false;
  }

  std::ifstream in(file_path, std::ios::binary);
  if (!in) {
    return false;
  }

  std::array<char, kHeaderSize> header{};
  in.read(header.data(), header.size());
  if (in.gcount() != static_cast<std::streamsize>(header.size()) ||
      std::memcmp(header.data(), kMagic, sizeof(kMagic)) != 0 ||
      ReadU32(header.data() + 8) != kFormatVersion) {
    return false;
  }

  std::error_code ec;
  const uint64_t actual_size = std::filesystem::file_size(file_path, ec);
  const uint32_t section_count = ReadU32(header.data() + 12);
  const uint64_t section_table_offset = ReadU64(header.data() + 16);
  if (ec || ReadU64(header.data() + 32) != actual_size ||
      section_count == 0 || section_table_offset < kHeaderSize ||
      !InRange(section_table_offset,
               static_cast<uint64_t>(section_count) * kSectionRecordSize,
               actual_size)) {
    return false;
  }

  *build_id = ReadU64(header.data() + 24);
  return true;
}

class RewriteMappedFile : public MappedFile {
 public:
  explicit RewriteMappedFile(const path& file_path) : MappedFile(file_path) {}

  bool Open() { return OpenReadOnly(); }
  const char* data() const { return address(); }
};

struct StageView {
  marisa::Trie trie;
  const char* key_index = nullptr;
  const char* value_pool = nullptr;
  const char* value_pool_end = nullptr;
  uint32_t key_count = 0;

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

  bool Lookup(std::string_view text,
              marisa::Agent* agent,
              vector<string>* values) const {
    if (!agent || !values) {
      return false;
    }
    values->clear();
    agent->set_query(text.data(), text.size());
    if (!trie.lookup(*agent)) {
      return false;
    }

    const char* cursor = nullptr;
    const char* end = nullptr;
    uint32_t value_count = 0;
    if (!GetValueList(agent->key().id(), &cursor, &end, &value_count)) {
      return false;
    }

    // Exact lookup must preserve every value in source order. Reserve only a
    // small initial capacity so a damaged pack cannot force a huge allocation
    // before the bounded value decoder has inspected the pool.
    values->reserve(std::min<uint32_t>(value_count, 16));
    for (uint32_t i = 0; i < value_count; ++i) {
      std::string_view value;
      if (!ReadValue(&cursor, end, &value)) {
        values->clear();
        return false;
      }
      values->emplace_back(value);
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

    if (!ReadValue(&cursor, end, value)) {
      return false;
    }
    return true;
  }

  bool ValidateValues() const {
    for (uint32_t key_id = 0; key_id < key_count; ++key_id) {
      const char* cursor = nullptr;
      const char* end = nullptr;
      uint32_t value_count = 0;
      if (!GetValueList(key_id, &cursor, &end, &value_count)) {
        return false;
      }
      for (uint32_t i = 0; i < value_count; ++i) {
        std::string_view value;
        if (!ReadValue(&cursor, end, &value)) {
          return false;
        }
      }
    }
    return true;
  }

  bool ConvertSentence(const string& text, string* result) const {
    result->clear();

    bool changed = false;
    size_t pos = 0;
    marisa::Agent agent;

    while (pos < text.size()) {
      agent.set_query(text.data() + pos, text.size() - pos);

      size_t longest_id = kInvalidIndex;
      size_t longest_size = 0;
      while (trie.common_prefix_search(agent)) {
        const size_t key_size = agent.key().length();
        if (key_size > longest_size) {
          longest_id = agent.key().id();
          longest_size = key_size;
        }
      }

      if (longest_id != kInvalidIndex) {
        std::string_view value;
        if (GetFirstValue(longest_id, &value)) {
          const bool replacement_changed =
              value.size() != longest_size ||
              std::memcmp(value.data(), text.data() + pos, longest_size) != 0;
          if (replacement_changed) {
            if (!changed) {
              result->reserve(text.size());
              result->assign(text.data(), pos);
            }
            changed = true;
          }
          if (changed && !replacement_changed) {
            result->append(text.data() + pos, longest_size);
          } else if (changed) {
            result->append(value.data(), value.size());
          }
          pos += longest_size;
          continue;
        }
      }

      const char* current = text.data() + pos;
      const size_t char_size =
          NextUtf8CharSize(current, text.data() + text.size());
      if (char_size == 0) {
        return changed;
      }
      if (changed) {
        result->append(current, char_size);
      }
      pos += char_size;
    }
    return changed;
  }
};

void WriteSectionRecord(std::ostream* out, const SectionRecord& record) {
  WriteU64(out, record.name_offset);
  WriteU32(out, record.name_size);
  WriteU32(out, record.stage_count);
  WriteU64(out, record.stage_table_offset);
}

void WriteStageRecord(std::ostream* out, const StageRecord& record) {
  WriteU64(out, record.trie_offset);
  WriteU64(out, record.trie_size);
  WriteU64(out, record.key_index_offset);
  WriteU32(out, record.key_count);
  WriteU32(out, 0);
  WriteU64(out, record.value_pool_offset);
  WriteU64(out, record.value_pool_size);
  WriteU64(out, 0);
  WriteU64(out, 0);
}

}  // namespace

class RewriteSection::Impl {
 public:
  // One section is one pipeline stage. Stage order belongs to engine/filters.
  StageView stage;
};

RewriteSection::RewriteSection() : impl_(new Impl) {}
RewriteSection::~RewriteSection() = default;

bool RewriteSection::Exact(const string& text, vector<string>* values) const {
  if (!values) {
    return false;
  }
  values->clear();
  if (text.empty()) {
    return false;
  }
  marisa::Agent agent;
  return impl_->stage.Lookup(text, &agent, values);
}

bool RewriteSection::ConvertSentence(const string& text, string* result) const {
  if (!result) {
    return false;
  }
  result->clear();
  if (text.empty()) {
    return false;
  }
  return impl_->stage.ConvertSentence(text, result);
}

class RewritePack::Impl {
 public:
  // Destroy stages and mapped name views before releasing the shared mapping.
  std::unique_ptr<RewriteMappedFile> file;
  // Built only by Open(). Each stage owns one independent mapped trie/view;
  // pointees stay at fixed addresses and the index is read-only after loading.
  std::unordered_map<std::string_view, std::unique_ptr<RewriteSection>> sections;
};

RewritePack::RewritePack(const path& file_path)
    : file_path_(file_path), impl_(new Impl) {}

RewritePack::~RewritePack() = default;

bool RewritePack::Open() {
  impl_->file = std::make_unique<RewriteMappedFile>(file_path_);
  if (!impl_->file->Open()) {
    LOG(ERROR) << "failed to map rewrite pack '" << file_path_ << "'.";
    return false;
  }

  const char* base = impl_->file->data();
  const uint64_t file_size = impl_->file->file_size();
  if (!base || file_size < kHeaderSize ||
      std::memcmp(base, kMagic, sizeof(kMagic)) != 0) {
    LOG(ERROR) << "invalid rewrite pack header: " << file_path_;
    return false;
  }

  const uint32_t version = ReadU32(base + 8);
  if (version != kFormatVersion) {
    LOG(ERROR) << "unsupported rewrite pack version " << version << ": "
               << file_path_;
    return false;
  }

  const uint32_t section_count = ReadU32(base + 12);
  const uint64_t section_table_offset = ReadU64(base + 16);
  build_id_ = ReadU64(base + 24);
  const uint64_t declared_file_size = ReadU64(base + 32);
  if (section_count == 0 || declared_file_size != file_size ||
      section_table_offset < kHeaderSize ||
      !InRange(section_table_offset,
               static_cast<uint64_t>(section_count) * kSectionRecordSize,
               file_size)) {
    LOG(ERROR) << "corrupted rewrite pack directory: " << file_path_;
    return false;
  }

  impl_->sections.clear();
  impl_->sections.reserve(section_count);

  for (uint32_t i = 0; i < section_count; ++i) {
    const char* p = base + section_table_offset + i * kSectionRecordSize;
    SectionRecord section_record;
    section_record.name_offset = ReadU64(p);
    section_record.name_size = ReadU32(p + 8);
    section_record.stage_count = ReadU32(p + 12);
    section_record.stage_table_offset = ReadU64(p + 16);

    if (!InRange(section_record.name_offset, section_record.name_size,
                 file_size) ||
        !InRange(section_record.stage_table_offset,
                 static_cast<uint64_t>(section_record.stage_count) *
                     kStageRecordSize,
                 file_size)) {
      LOG(ERROR) << "corrupted rewrite section directory entry.";
      return false;
    }

    const std::string_view name(base + section_record.name_offset,
                                section_record.name_size);
    if (name.empty() || section_record.stage_count != 1) {
      LOG(ERROR) << "invalid rewrite section directory entry.";
      return false;
    }
    if (impl_->sections.find(name) != impl_->sections.end()) {
      LOG(ERROR) << "duplicated rewrite section directory entry.";
      return false;
    }

    auto section = std::unique_ptr<RewriteSection>(new RewriteSection);
    const char* sp = base + section_record.stage_table_offset;
    StageRecord stage_record;
    stage_record.trie_offset = ReadU64(sp);
    stage_record.trie_size = ReadU64(sp + 8);
    stage_record.key_index_offset = ReadU64(sp + 16);
    stage_record.key_count = ReadU32(sp + 24);
    stage_record.value_pool_offset = ReadU64(sp + 32);
    stage_record.value_pool_size = ReadU64(sp + 40);

    if (stage_record.key_count == 0 || stage_record.trie_size == 0 ||
        stage_record.value_pool_size == 0 ||
        !InRange(stage_record.trie_offset, stage_record.trie_size, file_size) ||
        !InRange(stage_record.key_index_offset,
                 static_cast<uint64_t>(stage_record.key_count) *
                     kKeyIndexRecordSize,
                 file_size) ||
        !InRange(stage_record.value_pool_offset,
                 stage_record.value_pool_size, file_size)) {
      LOG(ERROR) << "corrupted rewrite stage in section '" << name << "'.";
      return false;
    }

    StageView& stage = section->impl_->stage;
    stage.key_index = base + stage_record.key_index_offset;
    stage.value_pool = base + stage_record.value_pool_offset;
    stage.value_pool_end = stage.value_pool + stage_record.value_pool_size;
    stage.key_count = stage_record.key_count;

    try {
      stage.trie.map(base + stage_record.trie_offset, stage_record.trie_size);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "failed to map trie in rewrite section '" << name
                 << "': " << ex.what();
      return false;
    } catch (...) {
      // marisa's public API is not required to use std::exception for every
      // malformed-input failure. A damaged pack must fail open cleanly.
      LOG(ERROR) << "failed to map trie in rewrite section '" << name
                 << "' with an unknown exception.";
      return false;
    }

    if (stage.trie.num_keys() != stage.key_count || !stage.ValidateValues()) {
      LOG(ERROR) << "corrupted rewrite stage in section '" << name << "'.";
      return false;
    }

    impl_->sections.emplace(name, std::move(section));
  }
  return true;
}

std::shared_ptr<RewritePack> RewritePack::OpenCached(const path& file_path) {
  static std::mutex mutex;
  static std::unordered_map<string, std::weak_ptr<RewritePack>> cache;

  // Use a stable UTF-8 key for cache identity. The path itself is still passed
  // to MappedFile, so native Unicode path handling remains platform-specific.
  const string cache_key = file_path.u8string();
  uint64_t on_disk_build_id = 0;
  const bool has_build_id = ReadPackBuildId(file_path, &on_disk_build_id);

  {
    // Mapping and validating MARISA tries can touch a large pack. Do not hold
    // the process-wide cache mutex while doing that work; concurrent misses
    // may build duplicate short-lived mappings, but startup is not serialized.
    std::lock_guard<std::mutex> lock(mutex);
    auto found = cache.find(cache_key);
    if (found != cache.end()) {
      if (auto cached = found->second.lock()) {
        if (has_build_id && cached->build_id() == on_disk_build_id) {
          return cached;
        }
      }
      cache.erase(found);
    }
  }

  auto pack = std::shared_ptr<RewritePack>(new RewritePack(file_path));
  if (!pack->Open()) {
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(mutex);
    auto found = cache.find(cache_key);
    if (found != cache.end()) {
      if (auto cached = found->second.lock()) {
        if (cached->build_id() == pack->build_id()) {
          return cached;
        }
      }
    }
    cache[cache_key] = pack;
  }
  return pack;
}

RewritePack::StageHandle RewritePack::FindSection(const string& name) const {
  const std::string_view wanted(name.data(), name.size());
  const auto& sections = impl_->sections;
  const auto found = sections.find(wanted);
  return found == sections.end() ? nullptr : found->second.get();
}

bool RewritePackBuilder::Build(
    const path& output_path,
    const vector<RewriteSectionData>& sections) const {
  if (sections.empty() || !FitsU32(sections.size())) {
    LOG(ERROR) << "rewrite pack requires a non-empty section list.";
    return false;
  }

  std::unordered_set<string> section_names;
  section_names.reserve(sections.size());
  for (const auto& section : sections) {
    if (section.name.empty() || !FitsU32(section.name.size()) ||
        section.stage.entries.empty() ||
        !section_names.insert(section.name).second) {
      LOG(ERROR) << "rewrite section has no usable entries or is duplicated: "
                 << section.name;
      return false;
    }
  }

  const uint64_t build_id = ComputeBuildId(sections);
  // Always emit a fresh pack. A matching build id only describes the source
  // entries; it cannot prove that an existing MARISA body was not truncated
  // or modified in place. Re-opening that untrusted body during deployment
  // can crash inside marisa::Trie::map before we get a C++ exception. The
  // deployment path is cold, while the runtime keeps its mmap/cache fast.

  std::error_code ec;
  const path parent_path = output_path.parent_path();
  if (!parent_path.empty()) {
    std::filesystem::create_directories(parent_path, ec);
    if (ec) {
      LOG(ERROR) << "failed to create rewrite pack directory: "
                 << ec.message();
      return false;
    }
  }

  const path temporary_path = MakeTemporaryPath(output_path);
  TemporaryFileGuard temporary_file(temporary_path);
  std::ofstream out(temporary_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    LOG(ERROR) << "failed to create rewrite pack: " << temporary_path;
    return false;
  }

  out.write(kMagic, sizeof(kMagic));
  WriteU32(&out, kFormatVersion);
  WriteU32(&out, static_cast<uint32_t>(sections.size()));
  WriteU64(&out, kHeaderSize);
  WriteU64(&out, build_id);
  WriteU64(&out, 0);

  const uint64_t section_table_offset = Tell(&out);
  WriteZeros(&out, sections.size() * kSectionRecordSize);
  vector<SectionRecord> section_records(sections.size());

  for (size_t section_index = 0; section_index < sections.size();
       ++section_index) {
    const auto& source_section = sections[section_index];
    auto& section_record = section_records[section_index];

    section_record.name_offset = Tell(&out);
    section_record.name_size =
        static_cast<uint32_t>(source_section.name.size());
    out.write(source_section.name.data(), source_section.name.size());

    section_record.stage_count = 1;
    section_record.stage_table_offset = Tell(&out);
    WriteZeros(&out, kStageRecordSize);
    StageRecord stage_record;
    const auto& source_stage = source_section.stage;

    // Views point into immutable source_stage.entries, avoiding a second
    // copy of every key/value while compiling. A tiny linear scan dedups
    // values per key; real rewrite dictionaries overwhelmingly have one
    // value per key, so this is cheaper than an unordered_set per bucket.
    struct Bucket {
      std::string_view key;
      vector<std::string_view> values;
    };

    vector<Bucket> buckets;
    std::unordered_map<std::string_view, size_t> key_to_bucket;
    buckets.reserve(source_stage.entries.size());
    key_to_bucket.reserve(source_stage.entries.size());

    for (const auto& entry : source_stage.entries) {
      if (entry.key.empty() || entry.value.empty()) {
        continue;
      }

      const std::string_view key(entry.key.data(), entry.key.size());
      const std::string_view value(entry.value.data(), entry.value.size());
      auto found = key_to_bucket.find(key);
      size_t bucket_index = 0;
      if (found == key_to_bucket.end()) {
        bucket_index = buckets.size();
        key_to_bucket.emplace(key, bucket_index);
        buckets.push_back(Bucket{key, {}});
      } else {
        bucket_index = found->second;
      }

      auto& values = buckets[bucket_index].values;
      if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
      }
    }

    if (buckets.empty() || !FitsU32(buckets.size())) {
      LOG(ERROR)
          << "rewrite stage has no usable keys or exceeds format limits.";
      return false;
    }

    // Use MARISA directly. The Keyset receives each unique key once;
    // after build(), MARISA writes the generated key id back to Keyset.
    // This avoids StringTableBuilder's reference array and its intermediate
    // stringstream/vector dump while preserving exactly the same trie model.
    marisa::Keyset keyset;
    for (const auto& bucket : buckets) {
      keyset.push_back(bucket.key.data(), bucket.key.size(), 1.0f);
    }

    marisa::Trie trie;
    try {
      LOG(INFO) << "building rewrite trie '" << source_section.name
                << "' with " << buckets.size() << " keys.";
      trie.build(keyset);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "failed to build rewrite trie for section '"
                 << source_section.name << "': " << ex.what();
      return false;
    } catch (...) {
      LOG(ERROR) << "failed to build rewrite trie for section '"
                 << source_section.name << "' with an unknown exception.";
      return false;
    }

    if (trie.num_keys() != buckets.size()) {
      LOG(ERROR) << "rewrite trie key count mismatch in section '"
                 << source_section.name << "'.";
      return false;
    }

    vector<size_t> bucket_by_key_id(buckets.size(), kInvalidIndex);
    for (size_t i = 0; i < buckets.size(); ++i) {
      const size_t key_id = keyset[i].id();
      if (key_id >= bucket_by_key_id.size() ||
          bucket_by_key_id[key_id] != kInvalidIndex) {
        LOG(ERROR) << "failed to assign rewrite trie key id.";
        return false;
      }
      bucket_by_key_id[key_id] = i;
    }

    // The RWP itself owns the single OS mapping. Serialize MARISA directly
    // into the pack stream instead of materializing another trie-sized byte
    // vector in deployment memory.
    Align(&out, alignof(uint64_t));
    stage_record.trie_offset = Tell(&out);
    stage_record.trie_size = trie.io_size();
    try {
      marisa::write(out, trie);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "failed to serialize rewrite trie for section '"
                 << source_section.name << "': " << ex.what();
      return false;
    } catch (...) {
      LOG(ERROR) << "failed to serialize rewrite trie for section '"
                 << source_section.name << "' with an unknown exception.";
      return false;
    }
    if (!out || Tell(&out) != stage_record.trie_offset +
                                 stage_record.trie_size) {
      LOG(ERROR) << "rewrite trie serialization size mismatch in section '"
                 << source_section.name << "'.";
      return false;
    }

    stage_record.key_index_offset = Tell(&out);
    stage_record.key_count = static_cast<uint32_t>(buckets.size());

    uint64_t value_pool_size = 0;
    for (size_t key_id = 0; key_id < bucket_by_key_id.size(); ++key_id) {
      const size_t bucket_index = bucket_by_key_id[key_id];
      if (bucket_index == kInvalidIndex) {
        LOG(ERROR) << "rewrite trie contains an unmapped key id.";
        return false;
      }

      const auto& values = buckets[bucket_index].values;
      if (values.empty() || !FitsU32(values.size()) ||
          value_pool_size > std::numeric_limits<uint32_t>::max()) {
        LOG(ERROR) << "rewrite value list exceeds format limits.";
        return false;
      }

      WriteU32(&out, static_cast<uint32_t>(value_pool_size));
      WriteU32(&out, static_cast<uint32_t>(values.size()));

      for (const auto value : values) {
        if (!FitsU32(value.size())) {
          LOG(ERROR) << "rewrite value exceeds format limits.";
          return false;
        }
        const uint64_t encoded_size =
            VarUint32Size(static_cast<uint32_t>(value.size())) +
            static_cast<uint64_t>(value.size());
        if (encoded_size > std::numeric_limits<uint32_t>::max() -
                               value_pool_size) {
          LOG(ERROR) << "rewrite value pool exceeds 4 GiB format limit.";
          return false;
        }
        value_pool_size += encoded_size;
      }
    }

    stage_record.value_pool_offset = Tell(&out);
    stage_record.value_pool_size = value_pool_size;
    for (size_t key_id = 0; key_id < bucket_by_key_id.size(); ++key_id) {
      const auto& values = buckets[bucket_by_key_id[key_id]].values;
      for (const auto value : values) {
        WriteVarUint32(&out, static_cast<uint32_t>(value.size()));
        out.write(value.data(), value.size());
      }
    }
    if (!out || Tell(&out) != stage_record.value_pool_offset +
                                 stage_record.value_pool_size) {
      LOG(ERROR) << "rewrite value pool serialization size mismatch.";
      return false;
    }
    const uint64_t return_pos = Tell(&out);
    out.seekp(section_record.stage_table_offset);
    if (!out) {
      LOG(ERROR) << "failed to seek to rewrite stage table.";
      return false;
    }
    WriteStageRecord(&out, stage_record);
    out.seekp(return_pos);
    if (!out) {
      LOG(ERROR) << "failed to restore rewrite pack write position.";
      return false;
    }
  }

  const uint64_t file_size = Tell(&out);
  out.seekp(section_table_offset);
  if (!out) {
    LOG(ERROR) << "failed to seek to rewrite section table.";
    return false;
  }
  for (const auto& section_record : section_records) {
    WriteSectionRecord(&out, section_record);
  }
  out.seekp(32);
  if (!out) {
    LOG(ERROR) << "failed to seek to rewrite pack header.";
    return false;
  }
  WriteU64(&out, file_size);
  out.close();

  if (!out) {
    return false;
  }

  // Rename within the same directory is atomic on POSIX. On Windows the
  // platform replacement API is used without deleting the old file first;
  // if a live mapping prevents replacement, deployment fails and the old
  // pack remains available.
  if (!InstallAtomically(temporary_path, output_path, &ec)) {
    LOG(ERROR) << "failed to install rewrite pack '" << output_path
               << "': " << ec.message();
    return false;
  }
  temporary_file.Commit();
  return true;
}

}  // namespace rime
