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
  TextDb(const string& name,
         const string& db_type,
         TextFormat format);
  virtual ~TextDb();

  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const string& snapshot_file);
  virtual bool Restore(const string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const string& key, string* value);
  virtual bool MetaUpdate(const string& key, const string& value);

  virtual an<DbAccessor> QueryMetadata();
  virtual an<DbAccessor> QueryAll();
  virtual an<DbAccessor> Query(const string& key);
  virtual bool Fetch(const string& key, string* value);
  virtual bool Update(const string& key, const string& value);
  virtual bool Erase(const string& key);

 protected:
  void Clear();
  bool LoadFromFile(const string& file);
  bool SaveToFile(const string& file);

  string db_type_;
  TextFormat format_;
  TextDbData metadata_;
  TextDbData data_;
  bool modified_ = false;
};

}  // namespace rime

#endif  // RIME_TEXT_DB_H_
