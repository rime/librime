//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TEXT_DB_H_
#define RIME_TEXT_DB_H_

#include <rime/dict/db.h>
#include <rime/dict/tsv.h>

namespace rime {

class TextDb;

using TextDbData = map<string, string>;

class TextDbAccessor : public DbAccessor {
 public:
  TextDbAccessor(const TextDbData& data, const string& prefix);
  virtual ~TextDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const string& key);
  virtual bool GetNextRecord(string* key, string* value);
  virtual bool exhausted();

 private:
  const TextDbData& data_;
  TextDbData::const_iterator iter_;
};

struct TextFormat {
  TsvParser parser;
  TsvFormatter formatter;
  string file_description;
};

class TextDb : public Db {
 public:
  TextDb(const path& file_path,
         const string& db_name,
         const string& db_type,
         TextFormat format);
  RIME_DLL virtual ~TextDb();

  RIME_DLL bool Open() override;
  RIME_DLL bool OpenReadOnly() override;
  RIME_DLL bool Close() override;

  RIME_DLL bool Backup(const path& snapshot_file) override;
  RIME_DLL bool Restore(const path& snapshot_file) override;

  RIME_DLL bool CreateMetadata() override;
  RIME_DLL bool MetaFetch(const string& key, string* value) override;
  RIME_DLL bool MetaUpdate(const string& key, const string& value) override;

  RIME_DLL an<DbAccessor> QueryMetadata() override;
  RIME_DLL an<DbAccessor> QueryAll() override;
  RIME_DLL an<DbAccessor> Query(const string& key) override;
  RIME_DLL bool Fetch(const string& key, string* value) override;
  RIME_DLL bool Update(const string& key, const string& value) override;
  RIME_DLL bool Erase(const string& key) override;

 protected:
  void Clear();
  bool LoadFromFile(const path& file);
  bool SaveToFile(const path& file);

  string db_type_;
  TextFormat format_;
  TextDbData metadata_;
  TextDbData data_;
  bool modified_ = false;
};

}  // namespace rime

#endif  // RIME_TEXT_DB_H_
