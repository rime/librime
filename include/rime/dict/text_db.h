//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TEXT_DB_H_
#define RIME_TEXT_DB_H_

#include <map>
#include <string>
#include <rime/dict/db.h>

namespace rime {

class TextDb;

typedef std::map<std::string, std::string> TextDbData;

class TextDbAccessor : public DbAccessor {
 public:
  TextDbAccessor(const TextDbData& data, const std::string& prefix);
  virtual ~TextDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const std::string &key);
  virtual bool GetNextRecord(std::string *key, std::string *value);
  virtual bool exhausted();

 private:
  const TextDbData& data_;
  TextDbData::const_iterator iter_;
};

class TextDb : public Db {
 public:
  TextDb(const std::string& name,
         const std::string& db_type = "",
         int key_fields = 1);
  virtual ~TextDb();

  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const std::string& snapshot_file);
  virtual bool Restore(const std::string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const std::string &key, std::string *value);
  virtual bool MetaUpdate(const std::string &key, const std::string &value);

  virtual shared_ptr<DbAccessor> Query(const std::string &key);
  virtual bool Fetch(const std::string &key, std::string *value);
  virtual bool Update(const std::string &key, const std::string &value);
  virtual bool Erase(const std::string &key);

 protected:
  bool LoadFromFile(const std::string& file);
  bool SaveToFile(const std::string& file);

  std::string db_type_;
  int key_fields_;
  TextDbData metadata_;
  TextDbData data_;
  bool modified_;
};

}  // namespace rime

#endif  // RIME_TEXT_DB_H_
