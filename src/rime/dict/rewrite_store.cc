//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/dict/rewrite_store.h>
#include "rewrite_internal.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rime {
namespace {

using rewrite_internal::AlignValue;
using rewrite_internal::FitsU32;
using rewrite_internal::FlushFileDurably;
using rewrite_internal::InRange;
using rewrite_internal::InstallAtomically;
using rewrite_internal::MakeTemporaryPath;
using rewrite_internal::ReadU32;
using rewrite_internal::ReadU64;
using rewrite_internal::StoreU32;
using rewrite_internal::StoreU64;
using rewrite_internal::TemporaryFileGuard;
using rewrite_internal::WriteU32;
using rewrite_internal::WriteU64;
using rewrite_internal::WriteZeros;

constexpr char kStoreMagic[] = {'R', 'I', 'M', 'E', 'R', 'W', 'S', '2'};
constexpr char kObjectMagic[] = {'R', 'I', 'M', 'E', 'R', 'W', 'O', '1'};
constexpr char kIndexMagic[] = {'R', 'I', 'M', 'E', 'R', 'W', 'I', '2'};
constexpr uint32_t kStoreVersion = 1;
constexpr uint32_t kObjectVersion = 1;
constexpr uint32_t kIndexVersion = 1;
constexpr uint64_t kSuperblockSize = 64;
constexpr uint64_t kSuperblockCount = 2;
constexpr uint64_t kStoreDataOffset = kSuperblockSize * kSuperblockCount;
constexpr uint64_t kObjectHeaderSize = 64;
constexpr uint64_t kIndexHeaderSize = 40;
constexpr uint64_t kIndexStageRecordSize = 32;

struct RewriteStageIdHash {
  size_t operator()(const RewriteStageId& id) const {
    uint64_t value = id.high;
    value ^= id.low + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
    return static_cast<size_t>(value);
  }
};

struct StoreSuperblock {
  uint64_t generation = 0;
  uint64_t index_offset = 0;
  uint64_t index_size = 0;
  uint64_t committed_size = 0;
  uint64_t index_hash = 0;
};

struct StageLocation {
  uint64_t object_offset = 0;
  uint64_t object_size = 0;
  uint64_t payload_offset = 0;
  uint64_t payload_size = 0;
  uint64_t payload_hash = 0;
};

struct StoreSnapshot {
  uint64_t generation = 0;
  uint64_t committed_size = 0;
  uint64_t index_offset = 0;
  uint64_t index_size = 0;
  uint64_t index_hash = 0;
  std::unordered_map<RewriteStageId, StageLocation, RewriteStageIdHash> stages;
  hash_map<string, vector<RewriteStageBinding>> schemas;
};

uint64_t HashBytes(uint64_t hash, const char* data, size_t size) {
  constexpr uint64_t kPrime = 1099511628211ULL;
  for (size_t i = 0; i < size; ++i) {
    hash ^= static_cast<unsigned char>(data[i]);
    hash *= kPrime;
  }
  return hash;
}

string StageIdString(const RewriteStageId& id) {
  static constexpr char kHex[] = "0123456789abcdef";
  string result(32, '0');
  for (size_t i = 0; i < 16; ++i) {
    const size_t high_shift = (15 - i) * 4;
    result[i] = kHex[(id.high >> high_shift) & 0x0f];
    result[16 + i] = kHex[(id.low >> high_shift) & 0x0f];
  }
  return result;
}

bool StageIdLess(const RewriteStageId& lhs, const RewriteStageId& rhs) {
  return lhs.high < rhs.high || (lhs.high == rhs.high && lhs.low < rhs.low);
}

uint64_t StoreHash(const char* data, size_t size) {
  return HashBytes(1469598103934665603ULL, data, size);
}

std::array<char, kSuperblockSize> SerializeSuperblock(
    const StoreSuperblock& block) {
  std::array<char, kSuperblockSize> bytes{};
  std::memcpy(bytes.data(), kStoreMagic, sizeof(kStoreMagic));
  StoreU32(bytes.data() + 8, kStoreVersion);
  StoreU32(bytes.data() + 12, 0);
  StoreU64(bytes.data() + 16, block.generation);
  StoreU64(bytes.data() + 24, block.index_offset);
  StoreU64(bytes.data() + 32, block.index_size);
  StoreU64(bytes.data() + 40, block.committed_size);
  StoreU64(bytes.data() + 48, block.index_hash);
  StoreU64(bytes.data() + 56, StoreHash(bytes.data(), 56));
  return bytes;
}

bool ParseSuperblock(const char* data,
                     uint64_t actual_size,
                     StoreSuperblock* block) {
  if (!data || !block ||
      std::memcmp(data, kStoreMagic, sizeof(kStoreMagic)) != 0 ||
      ReadU32(data + 8) != kStoreVersion ||
      ReadU64(data + 56) != StoreHash(data, 56)) {
    return false;
  }

  StoreSuperblock candidate;
  candidate.generation = ReadU64(data + 16);
  candidate.index_offset = ReadU64(data + 24);
  candidate.index_size = ReadU64(data + 32);
  candidate.committed_size = ReadU64(data + 40);
  candidate.index_hash = ReadU64(data + 48);
  if (candidate.generation == 0 || candidate.committed_size > actual_size ||
      candidate.committed_size < kStoreDataOffset ||
      candidate.index_size < kIndexHeaderSize ||
      candidate.index_offset < kStoreDataOffset ||
      !InRange(candidate.index_offset, candidate.index_size,
               candidate.committed_size)) {
    return false;
  }
  *block = candidate;
  return true;
}

bool InitializeStoreFile(const path& file_path) {
  std::error_code ec;
  const path parent = file_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      LOG(ERROR) << "cannot create rewrite store directory '" << parent
                 << "': " << ec.message();
      return false;
    }
  }

  std::ofstream out(static_cast<const std::filesystem::path&>(file_path),
                    std::ios::binary | std::ios::trunc);
  if (!out) {
    LOG(ERROR) << "cannot create shared rewrite store '" << file_path << "'.";
    return false;
  }
  WriteZeros(&out, kStoreDataOffset);
  out.close();
  return out && FlushFileDurably(file_path);
}

std::array<char, kObjectHeaderSize> SerializeObjectHeader(
    const RewriteStageId& id,
    uint64_t payload_size,
    uint64_t payload_hash) {
  std::array<char, kObjectHeaderSize> bytes{};
  std::memcpy(bytes.data(), kObjectMagic, sizeof(kObjectMagic));
  StoreU32(bytes.data() + 8, kObjectVersion);
  StoreU32(bytes.data() + 12, kObjectHeaderSize);
  StoreU64(bytes.data() + 16, id.high);
  StoreU64(bytes.data() + 24, id.low);
  StoreU64(bytes.data() + 32, payload_size);
  StoreU64(bytes.data() + 40, payload_hash);
  StoreU64(bytes.data() + 48, kObjectHeaderSize + payload_size);
  StoreU64(bytes.data() + 56, StoreHash(bytes.data(), 56));
  return bytes;
}

bool ReadObjectHeader(std::istream* in,
                      uint64_t object_offset,
                      uint64_t committed_size,
                      const RewriteStageId* expected_id,
                      StageLocation* location) {
  if (!in || !location ||
      !InRange(object_offset, kObjectHeaderSize, committed_size)) {
    return false;
  }

  std::array<char, kObjectHeaderSize> bytes{};
  in->clear();
  in->seekg(static_cast<std::streamoff>(object_offset));
  in->read(bytes.data(), bytes.size());
  if (!*in ||
      std::memcmp(bytes.data(), kObjectMagic, sizeof(kObjectMagic)) != 0 ||
      ReadU32(bytes.data() + 8) != kObjectVersion ||
      ReadU32(bytes.data() + 12) != kObjectHeaderSize ||
      ReadU64(bytes.data() + 56) != StoreHash(bytes.data(), 56)) {
    return false;
  }

  const RewriteStageId id{ReadU64(bytes.data() + 16),
                          ReadU64(bytes.data() + 24)};
  const uint64_t payload_size = ReadU64(bytes.data() + 32);
  const uint64_t payload_hash = ReadU64(bytes.data() + 40);
  const uint64_t object_size = ReadU64(bytes.data() + 48);
  if ((expected_id && id != *expected_id) || id.empty() ||
      object_size != kObjectHeaderSize + payload_size ||
      !InRange(object_offset, object_size, committed_size)) {
    return false;
  }

  location->object_offset = object_offset;
  location->object_size = object_size;
  location->payload_offset = object_offset + kObjectHeaderSize;
  location->payload_size = payload_size;
  location->payload_hash = payload_hash;
  return true;
}

bool SerializeStoreIndex(const StoreSnapshot& snapshot,
                         uint64_t generation,
                         string* bytes) {
  if (!bytes || !FitsU32(snapshot.stages.size()) ||
      !FitsU32(snapshot.schemas.size())) {
    return false;
  }

  vector<std::pair<RewriteStageId, StageLocation>> stages(
      snapshot.stages.begin(), snapshot.stages.end());
  std::sort(stages.begin(), stages.end(), [](const auto& lhs, const auto& rhs) {
    return StageIdLess(lhs.first, rhs.first);
  });

  vector<std::pair<string, vector<RewriteStageBinding>>> schemas(
      snapshot.schemas.begin(), snapshot.schemas.end());
  std::sort(
      schemas.begin(), schemas.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  std::ostringstream out(std::ios::binary | std::ios::out);
  out.write(kIndexMagic, sizeof(kIndexMagic));
  WriteU32(&out, kIndexVersion);
  WriteU32(&out, static_cast<uint32_t>(stages.size()));
  WriteU32(&out, static_cast<uint32_t>(schemas.size()));
  WriteU32(&out, 0);
  WriteU64(&out, generation);
  WriteU64(&out, 0);  // patched with total index size below

  for (const auto& stage : stages) {
    WriteU64(&out, stage.first.high);
    WriteU64(&out, stage.first.low);
    WriteU64(&out, stage.second.object_offset);
    WriteU64(&out, stage.second.object_size);
  }

  for (auto& schema : schemas) {
    auto& bindings = schema.second;
    std::sort(
        bindings.begin(), bindings.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.name < rhs.name; });
    if (!FitsU32(schema.first.size()) || !FitsU32(bindings.size())) {
      return false;
    }
    WriteU32(&out, static_cast<uint32_t>(schema.first.size()));
    WriteU32(&out, static_cast<uint32_t>(bindings.size()));
    out.write(schema.first.data(), schema.first.size());

    string previous_name;
    for (const auto& binding : bindings) {
      if (binding.name.empty() || binding.name == previous_name ||
          binding.stages.empty() || !FitsU32(binding.name.size()) ||
          !FitsU32(binding.stages.size())) {
        return false;
      }
      previous_name = binding.name;
      WriteU32(&out, static_cast<uint32_t>(binding.name.size()));
      WriteU32(&out, static_cast<uint32_t>(binding.stages.size()));
      out.write(binding.name.data(), binding.name.size());
      for (const auto& id : binding.stages) {
        if (id.empty() || snapshot.stages.find(id) == snapshot.stages.end()) {
          return false;
        }
        WriteU64(&out, id.high);
        WriteU64(&out, id.low);
      }
    }
  }

  *bytes = out.str();
  if (!out || bytes->size() < kIndexHeaderSize) {
    return false;
  }
  StoreU64(bytes->data() + 32, bytes->size());
  return true;
}

bool ParseStoreIndex(const path& file_path,
                     const StoreSuperblock& block,
                     StoreSnapshot* snapshot) {
  if (!snapshot || block.index_size > std::numeric_limits<size_t>::max() ||
      block.index_size < kIndexHeaderSize) {
    return false;
  }

  std::ifstream in(static_cast<const std::filesystem::path&>(file_path),
                   std::ios::binary);
  if (!in) {
    return false;
  }
  string bytes(static_cast<size_t>(block.index_size), '\0');
  in.seekg(static_cast<std::streamoff>(block.index_offset));
  in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!in || StoreHash(bytes.data(), bytes.size()) != block.index_hash ||
      std::memcmp(bytes.data(), kIndexMagic, sizeof(kIndexMagic)) != 0 ||
      ReadU32(bytes.data() + 8) != kIndexVersion ||
      ReadU64(bytes.data() + 24) != block.generation ||
      ReadU64(bytes.data() + 32) != bytes.size()) {
    return false;
  }

  const uint32_t stage_count = ReadU32(bytes.data() + 12);
  const uint32_t schema_count = ReadU32(bytes.data() + 16);
  const char* cursor = bytes.data() + kIndexHeaderSize;
  const char* const end = bytes.data() + bytes.size();

  auto take_u32 = [&](uint32_t* value) {
    if (!value || static_cast<size_t>(end - cursor) < sizeof(uint32_t)) {
      return false;
    }
    *value = ReadU32(cursor);
    cursor += sizeof(uint32_t);
    return true;
  };
  auto take_u64 = [&](uint64_t* value) {
    if (!value || static_cast<size_t>(end - cursor) < sizeof(uint64_t)) {
      return false;
    }
    *value = ReadU64(cursor);
    cursor += sizeof(uint64_t);
    return true;
  };
  auto take_string = [&](uint32_t size, string* value) {
    if (!value || size == 0 || static_cast<size_t>(end - cursor) < size) {
      return false;
    }
    value->assign(cursor, size);
    cursor += size;
    return true;
  };

  StoreSnapshot parsed;
  parsed.generation = block.generation;
  parsed.committed_size = block.committed_size;
  parsed.index_offset = block.index_offset;
  parsed.index_size = block.index_size;
  parsed.index_hash = block.index_hash;
  parsed.stages.reserve(stage_count);

  for (uint32_t i = 0; i < stage_count; ++i) {
    RewriteStageId id;
    StageLocation location;
    if (!take_u64(&id.high) || !take_u64(&id.low) ||
        !take_u64(&location.object_offset) ||
        !take_u64(&location.object_size) || id.empty() ||
        location.object_size < kObjectHeaderSize ||
        !InRange(location.object_offset, location.object_size,
                 block.committed_size)) {
      return false;
    }
    location.payload_offset = location.object_offset + kObjectHeaderSize;
    location.payload_size = location.object_size - kObjectHeaderSize;
    if (!parsed.stages.emplace(id, location).second) {
      return false;
    }
  }

  parsed.schemas.reserve(schema_count);
  for (uint32_t i = 0; i < schema_count; ++i) {
    uint32_t schema_name_size = 0;
    uint32_t binding_count = 0;
    string schema_name;
    if (!take_u32(&schema_name_size) || !take_u32(&binding_count) ||
        binding_count == 0 || !take_string(schema_name_size, &schema_name)) {
      return false;
    }

    vector<RewriteStageBinding> bindings;
    bindings.reserve(binding_count);
    string previous_name;
    for (uint32_t j = 0; j < binding_count; ++j) {
      uint32_t binding_name_size = 0;
      uint32_t binding_stage_count = 0;
      RewriteStageBinding binding;
      if (!take_u32(&binding_name_size) || !take_u32(&binding_stage_count) ||
          binding_stage_count == 0 ||
          !take_string(binding_name_size, &binding.name) ||
          (!previous_name.empty() && binding.name <= previous_name)) {
        return false;
      }
      previous_name = binding.name;
      binding.stages.reserve(binding_stage_count);
      for (uint32_t k = 0; k < binding_stage_count; ++k) {
        RewriteStageId id;
        if (!take_u64(&id.high) || !take_u64(&id.low) || id.empty() ||
            parsed.stages.find(id) == parsed.stages.end()) {
          return false;
        }
        binding.stages.push_back(id);
      }
      bindings.push_back(std::move(binding));
    }
    if (!parsed.schemas.emplace(std::move(schema_name), std::move(bindings))
             .second) {
      return false;
    }
  }

  if (cursor != end) {
    return false;
  }
  *snapshot = std::move(parsed);
  return true;
}

bool LoadStoreSnapshot(const path& file_path, StoreSnapshot* snapshot) {
  if (!snapshot) {
    return false;
  }
  std::error_code ec;
  const uint64_t actual_size = std::filesystem::file_size(file_path, ec);
  if (ec || actual_size < kStoreDataOffset) {
    return false;
  }

  std::ifstream in(static_cast<const std::filesystem::path&>(file_path),
                   std::ios::binary);
  if (!in) {
    return false;
  }
  std::array<char, kStoreDataOffset> header{};
  in.read(header.data(), header.size());
  if (!in) {
    return false;
  }

  vector<StoreSuperblock> candidates;
  for (uint64_t slot = 0; slot < kSuperblockCount; ++slot) {
    StoreSuperblock block;
    if (ParseSuperblock(header.data() + slot * kSuperblockSize, actual_size,
                        &block)) {
      candidates.push_back(block);
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.generation > rhs.generation;
            });
  for (const auto& block : candidates) {
    StoreSnapshot parsed;
    if (ParseStoreIndex(file_path, block, &parsed)) {
      *snapshot = std::move(parsed);
      return true;
    }
  }

  // A freshly created store has only the reserved superblock area.
  if (actual_size == kStoreDataOffset && candidates.empty()) {
    *snapshot = StoreSnapshot{};
    snapshot->committed_size = kStoreDataOffset;
    return true;
  }
  return false;
}

bool ReadStoreGeneration(const path& file_path, uint64_t* generation) {
  if (!generation) {
    return false;
  }
  std::error_code ec;
  const uint64_t actual_size = std::filesystem::file_size(file_path, ec);
  if (ec || actual_size < kStoreDataOffset) {
    return false;
  }

  std::ifstream in(static_cast<const std::filesystem::path&>(file_path),
                   std::ios::binary);
  if (!in) {
    return false;
  }
  std::array<char, kStoreDataOffset> header{};
  in.read(header.data(), header.size());
  if (!in) {
    return false;
  }

  uint64_t latest = 0;
  for (uint64_t slot = 0; slot < kSuperblockCount; ++slot) {
    StoreSuperblock block;
    if (ParseSuperblock(header.data() + slot * kSuperblockSize, actual_size,
                        &block)) {
      latest = std::max(latest, block.generation);
    }
  }
  if (latest == 0) {
    return false;
  }
  *generation = latest;
  return true;
}

bool WriteSuperblock(const path& file_path,
                     uint64_t slot,
                     const StoreSuperblock& block) {
  if (slot >= kSuperblockCount) {
    return false;
  }
  std::fstream io(static_cast<const std::filesystem::path&>(file_path),
                  std::ios::binary | std::ios::in | std::ios::out);
  if (!io) {
    return false;
  }
  const auto bytes = SerializeSuperblock(block);
  io.seekp(static_cast<std::streamoff>(slot * kSuperblockSize));
  io.write(bytes.data(), bytes.size());
  io.flush();
  io.close();
  return io && FlushFileDurably(file_path);
}

bool AppendIndexSnapshot(const path& file_path,
                         StoreSnapshot* snapshot,
                         uint64_t* write_end) {
  if (!snapshot || !write_end) {
    return false;
  }
  const uint64_t generation = snapshot->generation + 1;
  string index;
  if (!SerializeStoreIndex(*snapshot, generation, &index)) {
    return false;
  }

  std::fstream io(static_cast<const std::filesystem::path&>(file_path),
                  std::ios::binary | std::ios::in | std::ios::out);
  if (!io) {
    return false;
  }
  const uint64_t index_offset = AlignValue(*write_end, 8);
  io.seekp(static_cast<std::streamoff>(*write_end));
  WriteZeros(&io, static_cast<size_t>(index_offset - *write_end));
  io.write(index.data(), static_cast<std::streamsize>(index.size()));
  io.flush();
  if (!io) {
    return false;
  }
  const uint64_t committed_size = index_offset + index.size();
  io.close();
  if (!FlushFileDurably(file_path)) {
    return false;
  }

  StoreSuperblock block;
  block.generation = generation;
  block.index_offset = index_offset;
  block.index_size = index.size();
  block.committed_size = committed_size;
  block.index_hash = StoreHash(index.data(), index.size());
  if (!WriteSuperblock(file_path, generation % kSuperblockCount, block)) {
    return false;
  }

  snapshot->generation = generation;
  snapshot->index_offset = block.index_offset;
  snapshot->index_size = block.index_size;
  snapshot->index_hash = block.index_hash;
  snapshot->committed_size = committed_size;
  *write_end = committed_size;
  return true;
}

}  // namespace

class RewriteStore::Impl {
 public:
  StoreSnapshot snapshot;
};

RewriteStore::RewriteStore(const path& file_path)
    : file_path_(file_path), impl_(new Impl) {}

RewriteStore::~RewriteStore() = default;

bool RewriteStore::Open() {
  if (!LoadStoreSnapshot(file_path_, &impl_->snapshot) ||
      impl_->snapshot.generation == 0) {
    LOG(ERROR) << "shared rewrite store '" << file_path_
               << "' has no valid committed index; redeploy the workspace.";
    return false;
  }
  generation_ = impl_->snapshot.generation;
  return true;
}

std::shared_ptr<RewriteStore> RewriteStore::OpenCached(const path& file_path) {
  static std::mutex mutex;
  static hash_map<string, std::weak_ptr<RewriteStore>> cache;

  uint64_t generation = 0;
  if (!ReadStoreGeneration(file_path, &generation)) {
    return nullptr;
  }
  const string key = file_path.u8string();
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto found = cache.find(key);
    if (found != cache.end()) {
      if (auto cached = found->second.lock()) {
        if (cached->generation() == generation) {
          return cached;
        }
      }
      cache.erase(found);
    }
  }

  auto store = std::shared_ptr<RewriteStore>(new RewriteStore(file_path));
  if (!store->Open()) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto found = cache.find(key);
    if (found != cache.end()) {
      if (auto cached = found->second.lock()) {
        if (cached->generation() == store->generation()) {
          return cached;
        }
      }
    }
    cache[key] = store;
  }
  return store;
}

bool RewriteStore::FindStageSequence(const string& schema_id,
                                     const string& section_name,
                                     vector<RewriteStageId>* stage_ids) const {
  if (!stage_ids) {
    return false;
  }
  stage_ids->clear();
  const auto schema = impl_->snapshot.schemas.find(schema_id);
  if (schema == impl_->snapshot.schemas.end()) {
    return false;
  }
  for (const auto& binding : schema->second) {
    if (binding.name == section_name) {
      *stage_ids = binding.stages;
      return !stage_ids->empty();
    }
  }
  return false;
}

std::shared_ptr<const RewritePack> RewriteStore::OpenStage(
    const RewriteStageId& stage_id) const {
  const auto found = impl_->snapshot.stages.find(stage_id);
  if (found == impl_->snapshot.stages.end()) {
    return nullptr;
  }

  static std::mutex mutex;
  static hash_map<string, std::weak_ptr<const RewritePack>> cache;
  const string cache_key =
      file_path_.u8string() + "#" + StageIdString(stage_id);
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto cached = cache.find(cache_key);
    if (cached != cache.end()) {
      if (auto pack = cached->second.lock()) {
        return pack;
      }
      cache.erase(cached);
    }
  }

  const StageLocation& location = found->second;
  std::ifstream in(static_cast<const std::filesystem::path&>(file_path_),
                   std::ios::binary);
  StageLocation checked;
  if (!in ||
      !ReadObjectHeader(&in, location.object_offset,
                        impl_->snapshot.committed_size, &stage_id, &checked) ||
      checked.object_size != location.object_size) {
    LOG(ERROR) << "shared rewrite stage " << StageIdString(stage_id)
               << " has an invalid object header; redeploy the workspace.";
    return nullptr;
  }

  auto pack = std::shared_ptr<RewritePack>(new RewritePack(
      file_path_, checked.payload_offset, checked.payload_size));
  if (!pack->Open()) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto cached = cache.find(cache_key);
    if (cached != cache.end()) {
      if (auto existing = cached->second.lock()) {
        return existing;
      }
    }
    for (auto it = cache.begin(); it != cache.end();) {
      if (it->second.expired()) {
        it = cache.erase(it);
      } else {
        ++it;
      }
    }
    cache[cache_key] = pack;
  }
  return pack;
}

namespace {

std::unordered_set<RewriteStageId, RewriteStageIdHash> LiveStageIds(
    const StoreSnapshot& snapshot) {
  std::unordered_set<RewriteStageId, RewriteStageIdHash> ids;
  for (const auto& schema : snapshot.schemas) {
    for (const auto& binding : schema.second) {
      ids.insert(binding.stages.begin(), binding.stages.end());
    }
  }
  return ids;
}

bool ShouldCompact(const StoreSnapshot& snapshot) {
  if (snapshot.schemas.empty() || snapshot.committed_size <= kStoreDataOffset) {
    return false;
  }

  const auto live_ids = LiveStageIds(snapshot);
  StoreSnapshot compacted = snapshot;
  for (auto it = compacted.stages.begin(); it != compacted.stages.end();) {
    if (live_ids.find(it->first) == live_ids.end()) {
      it = compacted.stages.erase(it);
    } else {
      ++it;
    }
  }

  uint64_t compacted_size = kStoreDataOffset;
  for (const auto& id : live_ids) {
    const auto found = snapshot.stages.find(id);
    if (found == snapshot.stages.end()) {
      return true;
    }
    compacted_size = AlignValue(compacted_size, 8) + found->second.object_size;
  }
  string index;
  if (!SerializeStoreIndex(compacted, snapshot.generation + 1, &index)) {
    return true;
  }
  compacted_size = AlignValue(compacted_size, 8) + index.size();
  if (snapshot.committed_size <= compacted_size) {
    return false;
  }

  const uint64_t garbage = snapshot.committed_size - compacted_size;
  const size_t dead_stages = snapshot.stages.size() - live_ids.size();
  constexpr uint64_t kMinGarbageBytes = 4 * 1024 * 1024;
  return dead_stages >= 16 || (garbage >= kMinGarbageBytes &&
                               garbage >= snapshot.committed_size / 4);
}

bool CompactStoreFile(const path& file_path,
                      const StoreSnapshot& snapshot,
                      uint64_t source_end,
                      StoreSnapshot* compacted_out) {
  if (!compacted_out) {
    return false;
  }
  const auto live_ids = LiveStageIds(snapshot);
  for (const auto& id : live_ids) {
    if (snapshot.stages.find(id) == snapshot.stages.end()) {
      return false;
    }
  }

  const path temporary_path = MakeTemporaryPath(file_path);
  TemporaryFileGuard temporary_file(temporary_path);
  if (!InitializeStoreFile(temporary_path)) {
    return false;
  }

  std::ifstream source(static_cast<const std::filesystem::path&>(file_path),
                       std::ios::binary);
  std::fstream target(static_cast<const std::filesystem::path&>(temporary_path),
                      std::ios::binary | std::ios::in | std::ios::out);
  if (!source || !target) {
    return false;
  }

  StoreSnapshot compacted;
  compacted.generation = snapshot.generation;
  compacted.committed_size = kStoreDataOffset;
  compacted.schemas = snapshot.schemas;
  uint64_t write_end = kStoreDataOffset;

  vector<RewriteStageId> ordered_ids(live_ids.begin(), live_ids.end());
  std::sort(ordered_ids.begin(), ordered_ids.end(), StageIdLess);
  std::array<char, 64 * 1024> buffer{};
  for (const auto& id : ordered_ids) {
    const StageLocation& old_location = snapshot.stages.at(id);
    StageLocation checked;
    if (!ReadObjectHeader(&source, old_location.object_offset, source_end, &id,
                          &checked) ||
        checked.object_size != old_location.object_size) {
      return false;
    }

    const uint64_t new_offset = AlignValue(write_end, 8);
    target.seekp(static_cast<std::streamoff>(write_end));
    WriteZeros(&target, static_cast<size_t>(new_offset - write_end));
    source.clear();
    source.seekg(static_cast<std::streamoff>(old_location.object_offset));

    uint64_t remaining = old_location.object_size;
    uint64_t copied = 0;
    uint64_t payload_hash = 1469598103934665603ULL;
    while (remaining > 0) {
      const size_t chunk =
          static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
      source.read(buffer.data(), static_cast<std::streamsize>(chunk));
      if (!source) {
        return false;
      }
      target.write(buffer.data(), static_cast<std::streamsize>(chunk));
      if (!target) {
        return false;
      }
      const uint64_t chunk_begin = copied;
      const uint64_t chunk_end = copied + chunk;
      if (chunk_end > kObjectHeaderSize) {
        const size_t hash_begin = static_cast<size_t>(
            chunk_begin < kObjectHeaderSize ? kObjectHeaderSize - chunk_begin
                                            : 0);
        payload_hash = HashBytes(payload_hash, buffer.data() + hash_begin,
                                 chunk - hash_begin);
      }
      copied += chunk;
      remaining -= chunk;
    }
    if (payload_hash != checked.payload_hash) {
      LOG(ERROR) << "cannot compact shared rewrite stage " << StageIdString(id)
                 << ": payload checksum mismatch.";
      return false;
    }

    StageLocation location = checked;
    location.object_offset = new_offset;
    location.payload_offset = new_offset + kObjectHeaderSize;
    compacted.stages.emplace(id, location);
    write_end = new_offset + location.object_size;
  }

  target.flush();
  target.close();
  source.close();
  if (!FlushFileDurably(temporary_path) ||
      !AppendIndexSnapshot(temporary_path, &compacted, &write_end)) {
    return false;
  }

  StoreSuperblock mirror;
  mirror.generation = compacted.generation;
  mirror.index_offset = compacted.index_offset;
  mirror.index_size = compacted.index_size;
  mirror.committed_size = compacted.committed_size;
  mirror.index_hash = compacted.index_hash;
  if (!WriteSuperblock(temporary_path,
                       (compacted.generation + 1) % kSuperblockCount, mirror)) {
    return false;
  }

  std::error_code ec;
  if (!InstallAtomically(temporary_path, file_path, &ec)) {
    LOG(ERROR) << "cannot compact shared rewrite store '" << file_path
               << "': " << ec.message();
    return false;
  }
  temporary_file.Commit();
  *compacted_out = std::move(compacted);
  return true;
}

struct RewriteWorkspaceSnapshot {
  bool had_file = false;
  bool dirty = false;
  bool compact = false;
  bool failed = false;
  uint64_t committed_size = 0;
  std::array<char, kStoreDataOffset> header{};
  StoreSnapshot working;
  uint64_t write_end = kStoreDataOffset;
};

std::mutex& RewriteWorkspaceMutex() {
  static std::mutex mutex;
  return mutex;
}

hash_map<string, std::shared_ptr<RewriteWorkspaceSnapshot>>&
RewriteWorkspaces() {
  static hash_map<string, std::shared_ptr<RewriteWorkspaceSnapshot>> workspaces;
  return workspaces;
}

std::shared_ptr<RewriteWorkspaceSnapshot> FindRewriteWorkspace(
    const path& file_path) {
  std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
  const auto found = RewriteWorkspaces().find(file_path.u8string());
  return found == RewriteWorkspaces().end() ? nullptr : found->second;
}

bool RestoreRewriteWorkspace(const path& file_path,
                             const RewriteWorkspaceSnapshot& snapshot) {
  std::error_code ec;
  if (!snapshot.had_file) {
    std::filesystem::remove(file_path, ec);
    return !ec;
  }
  if (!std::filesystem::exists(file_path, ec) || ec) {
    return false;
  }
  std::filesystem::resize_file(file_path, snapshot.committed_size, ec);
  if (ec) {
    return false;
  }
  std::fstream io(static_cast<const std::filesystem::path&>(file_path),
                  std::ios::binary | std::ios::in | std::ios::out);
  if (!io) {
    return false;
  }
  io.seekp(0);
  io.write(snapshot.header.data(), snapshot.header.size());
  io.flush();
  io.close();
  return io && FlushFileDurably(file_path);
}

void FinishRewriteWorkspace(const path& file_path) {
  std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
  RewriteWorkspaces().erase(file_path.u8string());
}

}  // namespace

class RewriteStoreWriter::Impl {
 public:
  explicit Impl(path path_value) : file_path(std::move(path_value)) {}

  StoreSnapshot& Snapshot() {
    return workspace ? workspace->working : snapshot;
  }
  const StoreSnapshot& Snapshot() const {
    return workspace ? workspace->working : snapshot;
  }
  uint64_t& WriteEnd() { return workspace ? workspace->write_end : write_end; }
  uint64_t WriteEnd() const {
    return workspace ? workspace->write_end : write_end;
  }

  path file_path;
  StoreSnapshot snapshot;
  uint64_t write_end = kStoreDataOffset;
  std::shared_ptr<RewriteWorkspaceSnapshot> workspace;
  bool opened = false;
};

RewriteStoreWriter::RewriteStoreWriter(const path& file_path)
    : impl_(new Impl(file_path)) {}

RewriteStoreWriter::~RewriteStoreWriter() = default;

bool RewriteStoreWriter::Open() {
  if (impl_->opened) {
    return true;
  }

  std::error_code ec;
  if (!std::filesystem::exists(impl_->file_path, ec)) {
    if (ec || !InitializeStoreFile(impl_->file_path)) {
      return false;
    }
  } else {
    const uint64_t size = std::filesystem::file_size(impl_->file_path, ec);
    if (ec) {
      return false;
    }
    if (size == 0 && !InitializeStoreFile(impl_->file_path)) {
      return false;
    }
  }

  impl_->workspace = FindRewriteWorkspace(impl_->file_path);
  if (impl_->workspace) {
    const uint64_t actual_size =
        std::filesystem::file_size(impl_->file_path, ec);
    if (ec) {
      return false;
    }
    if (actual_size < impl_->WriteEnd()) {
      LOG(ERROR) << "shared rewrite store '" << impl_->file_path
                 << "' was truncated during workspace deployment.";
      return false;
    }
    if (actual_size > impl_->WriteEnd()) {
      std::filesystem::resize_file(impl_->file_path, impl_->WriteEnd(), ec);
      if (ec) {
        LOG(ERROR) << "cannot discard an unfinished rewrite-stage tail from '"
                   << impl_->file_path << "': " << ec.message();
        return false;
      }
    }
    impl_->opened = true;
    return true;
  }

  if (!LoadStoreSnapshot(impl_->file_path, &impl_->snapshot)) {
    LOG(ERROR) << "shared rewrite store '" << impl_->file_path
               << "' is corrupted; remove it and redeploy the workspace.";
    return false;
  }

  const uint64_t actual_size = std::filesystem::file_size(impl_->file_path, ec);
  if (ec) {
    return false;
  }
  if (actual_size > impl_->snapshot.committed_size) {
    std::filesystem::resize_file(impl_->file_path,
                                 impl_->snapshot.committed_size, ec);
    if (ec) {
      LOG(ERROR) << "cannot discard an uncommitted rewrite-store tail from '"
                 << impl_->file_path << "': " << ec.message();
      return false;
    }
  }

  impl_->write_end = impl_->snapshot.committed_size;
  impl_->opened = true;
  return true;
}

bool RewriteStoreWriter::BeginWorkspace() {
  std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
  const string key = impl_->file_path.u8string();
  if (RewriteWorkspaces().find(key) != RewriteWorkspaces().end()) {
    LOG(ERROR) << "rewrite workspace transaction already active for '"
               << impl_->file_path << "'.";
    return false;
  }

  auto snapshot = std::make_shared<RewriteWorkspaceSnapshot>();
  std::error_code ec;
  if (std::filesystem::exists(impl_->file_path, ec)) {
    if (ec) {
      return false;
    }
    StoreSnapshot current;
    if (!LoadStoreSnapshot(impl_->file_path, &current)) {
      return false;
    }
    snapshot->had_file = true;
    snapshot->compact = ShouldCompact(current);
    snapshot->committed_size = current.committed_size;
    {
      std::ifstream in(
          static_cast<const std::filesystem::path&>(impl_->file_path),
          std::ios::binary);
      in.read(snapshot->header.data(), snapshot->header.size());
      if (!in) {
        return false;
      }
    }
    snapshot->working = current;
    snapshot->write_end = current.committed_size;
    const uint64_t actual_size =
        std::filesystem::file_size(impl_->file_path, ec);
    if (ec) {
      return false;
    }
    if (actual_size != current.committed_size) {
      std::filesystem::resize_file(impl_->file_path, current.committed_size,
                                   ec);
      if (ec || !FlushFileDurably(impl_->file_path)) {
        return false;
      }
    }
  }
  RewriteWorkspaces().emplace(key, std::move(snapshot));
  return true;
}

bool RewriteStoreWriter::AbortWorkspace() {
  std::shared_ptr<RewriteWorkspaceSnapshot> snapshot;
  {
    std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
    const string key = impl_->file_path.u8string();
    const auto found = RewriteWorkspaces().find(key);
    if (found == RewriteWorkspaces().end()) {
      return true;
    }
    snapshot = found->second;
    RewriteWorkspaces().erase(found);
  }
  if (!snapshot || !RestoreRewriteWorkspace(impl_->file_path, *snapshot)) {
    LOG(ERROR) << "failed to roll back shared rewrite store '"
               << impl_->file_path << "'.";
    return false;
  }
  impl_->opened = false;
  impl_->workspace.reset();
  impl_->snapshot = StoreSnapshot{};
  impl_->write_end = kStoreDataOffset;
  return true;
}

void RewriteStoreWriter::MarkWorkspaceFailed() {
  std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
  const auto found = RewriteWorkspaces().find(impl_->file_path.u8string());
  if (found != RewriteWorkspaces().end() && found->second) {
    found->second->failed = true;
  }
}

bool RewriteStoreWriter::WorkspaceFailed() const {
  std::lock_guard<std::mutex> lock(RewriteWorkspaceMutex());
  const auto found = RewriteWorkspaces().find(impl_->file_path.u8string());
  return found != RewriteWorkspaces().end() && found->second &&
         found->second->failed;
}

bool RewriteStoreWriter::HasStage(const RewriteStageId& stage_id) const {
  if (!impl_->opened) {
    return false;
  }
  const auto& snapshot = impl_->Snapshot();
  return snapshot.stages.find(stage_id) != snapshot.stages.end();
}

bool RewriteStoreWriter::AppendStage(const RewriteStageId& stage_id,
                                     RewriteStageData stage) {
  if (!impl_->opened || stage_id.empty() || stage.entries.empty()) {
    return false;
  }
  auto& snapshot = impl_->Snapshot();
  uint64_t& write_end = impl_->WriteEnd();
  if (HasStage(stage_id)) {
    return true;
  }

  const path payload_path = MakeTemporaryPath(impl_->file_path);
  TemporaryFileGuard payload_guard(payload_path);
  if (!RewritePackBuilder().Build(payload_path, stage)) {
    return false;
  }
  std::error_code ec;
  const uint64_t payload_size = std::filesystem::file_size(payload_path, ec);
  if (ec || payload_size == 0) {
    return false;
  }

  std::ifstream payload(static_cast<const std::filesystem::path&>(payload_path),
                        std::ios::binary);
  std::fstream store(
      static_cast<const std::filesystem::path&>(impl_->file_path),
      std::ios::binary | std::ios::in | std::ios::out);
  if (!payload || !store) {
    return false;
  }

  const uint64_t object_offset = AlignValue(write_end, 8);
  store.seekp(static_cast<std::streamoff>(write_end));
  WriteZeros(&store, static_cast<size_t>(object_offset - write_end));
  WriteZeros(&store, kObjectHeaderSize);

  uint64_t payload_hash = 1469598103934665603ULL;
  std::array<char, 64 * 1024> buffer{};
  uint64_t copied = 0;
  while (payload) {
    payload.read(buffer.data(), buffer.size());
    const std::streamsize size = payload.gcount();
    if (size > 0) {
      store.write(buffer.data(), size);
      payload_hash =
          HashBytes(payload_hash, buffer.data(), static_cast<size_t>(size));
      copied += static_cast<uint64_t>(size);
    }
  }
  if (!payload.eof() || !store || copied != payload_size) {
    return false;
  }

  const auto header =
      SerializeObjectHeader(stage_id, payload_size, payload_hash);
  store.seekp(static_cast<std::streamoff>(object_offset));
  store.write(header.data(), header.size());
  store.flush();
  if (!store) {
    return false;
  }
  store.close();

  StageLocation location;
  location.object_offset = object_offset;
  location.object_size = kObjectHeaderSize + payload_size;
  location.payload_offset = object_offset + kObjectHeaderSize;
  location.payload_size = payload_size;
  location.payload_hash = payload_hash;
  snapshot.stages.emplace(stage_id, location);
  write_end = object_offset + location.object_size;
  if (impl_->workspace) {
    impl_->workspace->dirty = true;
  }
  return true;
}

bool RewriteStoreWriter::CommitSchema(
    const string& schema_id,
    const vector<RewriteStageBinding>& bindings) {
  if (!impl_->opened || schema_id.empty()) {
    return false;
  }

  auto& snapshot = impl_->Snapshot();
  uint64_t& write_end = impl_->WriteEnd();
  vector<RewriteStageBinding> canonical = bindings;
  std::sort(
      canonical.begin(), canonical.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.name < rhs.name; });

  for (size_t i = 0; i < canonical.size(); ++i) {
    const auto& binding = canonical[i];
    if (binding.name.empty() || binding.stages.empty() ||
        (i != 0 && canonical[i - 1].name == binding.name)) {
      return false;
    }
    for (const auto& id : binding.stages) {
      if (id.empty() || snapshot.stages.find(id) == snapshot.stages.end()) {
        return false;
      }
    }
  }

  const auto same_bindings = [](const vector<RewriteStageBinding>& lhs,
                                const vector<RewriteStageBinding>& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
      if (lhs[i].name != rhs[i].name ||
          lhs[i].stages.size() != rhs[i].stages.size()) {
        return false;
      }
      for (size_t j = 0; j < lhs[i].stages.size(); ++j) {
        if (lhs[i].stages[j] != rhs[i].stages[j]) {
          return false;
        }
      }
    }
    return true;
  };

  bool changed = false;
  const auto current = snapshot.schemas.find(schema_id);
  if (canonical.empty()) {
    if (current != snapshot.schemas.end()) {
      snapshot.schemas.erase(current);
      changed = true;
    }
  } else if (current == snapshot.schemas.end() ||
             !same_bindings(current->second, canonical)) {
    snapshot.schemas[schema_id] = std::move(canonical);
    changed = true;
  }

  if (impl_->workspace) {
    impl_->workspace->dirty |= changed;
    return true;
  }
  if (!changed && write_end == snapshot.committed_size) {
    return true;
  }
  return AppendIndexSnapshot(impl_->file_path, &snapshot, &write_end);
}

bool RewriteStoreWriter::RetainSchemas(const vector<string>& schema_ids) {
  if (!impl_->opened) {
    return false;
  }
  auto& snapshot = impl_->Snapshot();
  uint64_t& write_end = impl_->WriteEnd();
  hash_set<string> live(schema_ids.begin(), schema_ids.end());
  bool changed = false;
  for (auto it = snapshot.schemas.begin(); it != snapshot.schemas.end();) {
    if (live.find(it->first) == live.end()) {
      it = snapshot.schemas.erase(it);
      changed = true;
    } else {
      ++it;
    }
  }
  if (impl_->workspace) {
    if (changed) {
      impl_->workspace->dirty = true;
    }
    return true;
  }
  if (!changed) {
    return true;
  }
  return AppendIndexSnapshot(impl_->file_path, &snapshot, &write_end);
}

bool RewriteStoreWriter::CommitWorkspace() {
  if (!impl_->opened || !impl_->workspace) {
    return false;
  }
  if (WorkspaceFailed()) {
    LOG(ERROR) << "shared rewrite-store transaction is marked failed; "
                  "refusing to commit partial rewrite data.";
    return false;
  }

  auto& snapshot = impl_->Snapshot();
  if (snapshot.schemas.empty()) {
    std::error_code ec;
    std::filesystem::remove(impl_->file_path, ec);
    if (ec) {
      LOG(ERROR) << "cannot remove empty shared rewrite store '"
                 << impl_->file_path << "': " << ec.message();
      return false;
    }
    FinishRewriteWorkspace(impl_->file_path);
    impl_->workspace.reset();
    impl_->opened = false;
    impl_->snapshot = StoreSnapshot{};
    impl_->write_end = kStoreDataOffset;
    return true;
  }

  const bool compact = impl_->workspace->compact || ShouldCompact(snapshot);
  if (!impl_->workspace->dirty && !compact) {
    impl_->snapshot = snapshot;
    impl_->write_end = impl_->WriteEnd();
    FinishRewriteWorkspace(impl_->file_path);
    impl_->workspace.reset();
    return true;
  }

  if (!compact) {
    uint64_t& write_end = impl_->WriteEnd();
    if (!AppendIndexSnapshot(impl_->file_path, &snapshot, &write_end)) {
      return false;
    }
    impl_->snapshot = snapshot;
    impl_->write_end = write_end;
    FinishRewriteWorkspace(impl_->file_path);
    impl_->workspace.reset();
    return true;
  }

  StoreSnapshot compacted;
  if (!CompactStoreFile(impl_->file_path, snapshot, impl_->WriteEnd(),
                        &compacted)) {
    return false;
  }
  FinishRewriteWorkspace(impl_->file_path);
  impl_->workspace.reset();
  impl_->snapshot = std::move(compacted);
  impl_->write_end = impl_->snapshot.committed_size;
  return true;
}

}  // namespace rime
