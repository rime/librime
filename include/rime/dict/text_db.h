//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TEXT_DB_H_
#define RIME_TEXT_DB_H_

#include <map>
#include <string>
#include <rime/dict/db.h>
#include <rime/dict/tsv.h>

namespace rime {

class TextDb;

using TextDbData = std::map<std::string, std::string>;

class TextDbAccessor : public DbAccessor {
 public:
  TextDbAccessor(const TextDbData& data, const std::string& prefix);
  virtual ~TextDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const std::string& key);
  virtual bool GetNextRecord(std::string* key, std::string* value);
  virtual bool exhausted();

 private:
  const TextDbData& data_;
  TextDbData::const_iterator iter_;
};

struct TextFormat {
  TsvParser parser;
  TsvFormatter formatter;
  std::string file_description;
};

class TextDb : public Db {
 public:
  TextDb(const std::string& name,
         const std::string& db_type,
         TextFormat format);
  virtual ~TextDb();

  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const std::string& snapshot_file);
  virtual bool Restore(const std::string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const std::string& key, std::string* value);
  virtual bool MetaUpdate(const std::string& key, const std::string& value);

  virtual shared_ptr<DbAccessor> QueryMetadata();
  virtual shared_ptr<DbAccessor> QueryAll();
  virtual shared_ptr<DbAccessor> Query(const std::string& key);
  virtual bool Fetch(const std::string& key, std::string* value);
  virtual bool Update(const std::string& key, const std::string& value);
  virtual bool Erase(const std::string& key);

 protected:
  void Clear();
  bool LoadFromFile(const std::string& file);
  bool SaveToFile(const std::string& file);

  std::string db_type_;
  TextFormat format_;
  TextDbData metadata_;
  TextDbData data_;
  bool modified_ = false;
};

}  // namespace rime

#endif  // RIME_TEXT_DB_H_
