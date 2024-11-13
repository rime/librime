//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#include <rime/dict/db_utils.h>
#include <rime/dict/text_db.h>

namespace rime {

// TextDbAccessor members

TextDbAccessor::TextDbAccessor(const TextDbData& data, string_view prefix)
    : DbAccessor(prefix), data_(data) {
  Reset();
}

TextDbAccessor::~TextDbAccessor() {}

bool TextDbAccessor::Reset() {
  iter_ = prefix_.empty() ? data_.begin() : data_.lower_bound(prefix_);
  return iter_ != data_.end();
}

bool TextDbAccessor::Jump(string_view key) {
  iter_ = data_.lower_bound(key);
  return iter_ != data_.end();
}

bool TextDbAccessor::GetNextRecord(string* key, string* value) {
  if (!key || !value || exhausted())
    return false;
  *key = iter_->first;
  *value = iter_->second;
  ++iter_;
  return true;
}

bool TextDbAccessor::exhausted() {
  return iter_ == data_.end() || !MatchesPrefix(iter_->first);
}

// TextDb members

TextDb::TextDb(const path& file_path,
               string_view db_name,
               string_view db_type,
               TextFormat format)
    : Db(file_path, db_name), db_type_(db_type), format_(format) {}

TextDb::~TextDb() {
  if (loaded())
    Close();
}

an<DbAccessor> TextDb::QueryMetadata() {
  if (!loaded())
    return nullptr;
  return New<TextDbAccessor>(metadata_, "");
}

an<DbAccessor> TextDb::QueryAll() {
  return Query("");
}

an<DbAccessor> TextDb::Query(string_view key) {
  if (!loaded())
    return nullptr;
  return New<TextDbAccessor>(data_, key);
}

bool TextDb::Fetch(string_view key, string* value) {
  if (!value || !loaded())
    return false;
  TextDbData::const_iterator it = data_.find(key);
  if (it == data_.end())
    return false;
  *value = it->second;
  return true;
}

bool TextDb::Update(string_view key, string_view value) {
  if (!loaded() || readonly())
    return false;
  DLOG(INFO) << "update db entry: " << key << " => " << value;
  data_[string{key}] = value;
  modified_ = true;
  return true;
}

bool TextDb::Erase(string_view key) {
  if (!loaded() || readonly())
    return false;
  DLOG(INFO) << "erase db entry: " << key;
  if (data_.erase(string{key}) == 0)
    return false;
  modified_ = true;
  return true;
}

bool TextDb::Open() {
  if (loaded())
    return false;
  loaded_ = true;
  readonly_ = false;
  loaded_ = !Exists() || LoadFromFile(file_path());
  if (loaded_) {
    string db_name;
    if (!MetaFetch("/db_name", &db_name)) {
      if (!CreateMetadata()) {
        LOG(ERROR) << "error creating metadata.";
        Close();
      }
    }
  } else {
    LOG(ERROR) << "Error opening db '" << name() << "'.";
  }
  modified_ = false;
  return loaded_;
}

bool TextDb::OpenReadOnly() {
  if (loaded())
    return false;
  loaded_ = true;
  readonly_ = false;
  loaded_ = Exists() && LoadFromFile(file_path());
  if (loaded_) {
    readonly_ = true;
  } else {
    LOG(ERROR) << "Error opening db '" << name_ << "' read-only.";
  }
  modified_ = false;
  return loaded_;
}

bool TextDb::Close() {
  if (!loaded())
    return false;
  if (modified_ && !SaveToFile(file_path())) {
    return false;
  }
  loaded_ = false;
  readonly_ = false;
  Clear();
  modified_ = false;
  return true;
}

void TextDb::Clear() {
  metadata_.clear();
  data_.clear();
}

bool TextDb::Backup(const path& snapshot_file) {
  if (!loaded())
    return false;
  LOG(INFO) << "backing up db '" << name() << "' to " << snapshot_file;
  if (!SaveToFile(snapshot_file)) {
    LOG(ERROR) << "failed to create snapshot file '" << snapshot_file
               << "' for db '" << name() << "'.";
    return false;
  }
  return true;
}

bool TextDb::Restore(const path& snapshot_file) {
  if (!loaded() || readonly())
    return false;
  if (!LoadFromFile(snapshot_file)) {
    LOG(ERROR) << "failed to restore db '" << name() << "' from '"
               << snapshot_file << "'.";
    return false;
  }
  modified_ = false;
  return true;
}

bool TextDb::CreateMetadata() {
  return Db::CreateMetadata() && MetaUpdate("/db_type", db_type_);
}

bool TextDb::MetaFetch(string_view key, string* value) {
  if (!value || !loaded())
    return false;
  TextDbData::const_iterator it = metadata_.find(key);
  if (it == metadata_.end())
    return false;
  *value = it->second;
  return true;
}

bool TextDb::MetaUpdate(string_view key, string_view value) {
  if (!loaded() || readonly())
    return false;
  DLOG(INFO) << "update db metadata: " << key << " => " << value;
  metadata_[string{key}] = value;
  modified_ = true;
  return true;
}

bool TextDb::LoadFromFile(const path& file) {
  Clear();
  TsvReader reader(file, format_.parser);
  DbSink sink(this);
  int entries = 0;
  try {
    entries = reader >> sink;
  } catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return false;
  }
  DLOG(INFO) << entries << " entries loaded.";
  return true;
}

bool TextDb::SaveToFile(const path& file) {
  TsvWriter writer(file, format_.formatter);
  writer.file_description = format_.file_description;
  DbSource source(this);
  int entries = 0;
  try {
    entries = writer << source;
  } catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return false;
  }
  DLOG(INFO) << entries << " entries saved.";
  return true;
}

}  // namespace rime
