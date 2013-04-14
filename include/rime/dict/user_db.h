//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// A simple wrapper for kyotocabinet::TreeDB
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4351)
#endif
#include <kchashdb.h>
#if defined(_MSC_VER)
#pragma warning(default: 4351)
#pragma warning(default: 4244)
#endif

#include <string>
#include <rime/common.h>

namespace rime {

class TreeDbAccessor {
 public:
  TreeDbAccessor() {}
  explicit TreeDbAccessor(kyotocabinet::DB::Cursor *cursor,
                          const std::string &prefix);
  ~TreeDbAccessor();

  bool Reset();
  bool Forward(const std::string &key);
  bool Backward(const std::string &key);
  bool GetNextRecord(std::string *key, std::string *value);
  bool exhausted();

 private:
  scoped_ptr<kyotocabinet::DB::Cursor> cursor_;
  std::string prefix_;
};

class TreeDb {
 public:
  TreeDb(const std::string &name);
  virtual ~TreeDb();

  bool Exists() const;
  bool Remove();
  bool Open();
  bool OpenReadOnly();
  bool Close();

  const shared_ptr<TreeDbAccessor> Query(const std::string &key);
  bool Fetch(const std::string &key, std::string *value);
  bool Update(const std::string &key, const std::string &value);
  bool Erase(const std::string &key);
  bool Backup();
  bool Recover();
  bool Restore(const std::string& snapshot_file);

  bool BeginTransaction();
  bool AbortTransaction();
  bool CommitTransaction();

  const std::string& name() const { return name_; }
  const std::string& file_name() const { return file_name_; }
  bool loaded() const { return loaded_; }
  bool in_transaction() const { return in_transaction_; }

  bool disabled() const { return disabled_; }
  void Disable() { disabled_ = true; }
  void Enable() { disabled_ = false; }

 protected:
  virtual bool CreateMetadata();
  void Initialize();

  std::string name_;
  std::string file_name_;
  bool loaded_;
  scoped_ptr<kyotocabinet::TreeDB> db_;
  bool in_transaction_;
  bool disabled_;
};

typedef TreeDbAccessor UserDbAccessor;

class UserDb : public TreeDb {
 public:
  UserDb(const std::string &name);
  virtual bool CreateMetadata();
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
