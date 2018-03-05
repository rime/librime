//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#include <stdint.h>
#include <rime/component.h>
#include <rime/dict/db.h>
#include <rime/dict/db_utils.h>

namespace rime {

using TickCount = uint64_t;

/// Properties of a user db entry value.
struct UserDbValue {
  int commits = 0;
  double dee = 0.0;
  TickCount tick = 0;

  UserDbValue() = default;
  UserDbValue(const string& value);

  string Pack() const;
  bool Unpack(const string& value);
};

/**
 * A placeholder class for user db.
 *
 * Note: do not directly use this class to instantiate a user db.
 * Instead, use the rime::UserDbWrapper<T> template, which creates
 * wrapper classes for underlying implementations of rime::Db.
 */
class UserDb {
 public:
  /// Abstract class for a user db component.
  class Component : public Db::Component {
   public:
    virtual string extension() const = 0;
    virtual string snapshot_extension() const = 0;
  };

  /// Requires a registered component for a user db class.
  static Component* Require(const string& name) {
    return static_cast<Component*>(Db::Require(name));
  }

  UserDb() = delete;
};

/// A helper class to provide extra functionalities related to user db.
class UserDbHelper {
 public:
  UserDbHelper(Db* db) : db_(db) {
  }
  UserDbHelper(const the<Db>& db) : db_(db.get()) {
  }
  UserDbHelper(const an<Db>& db) : db_(db.get()) {
  }

  bool UpdateUserInfo();
  static bool IsUniformFormat(const string& name);
  bool UniformBackup(const string& snapshot_file);
  bool UniformRestore(const string& snapshot_file);

  bool IsUserDb();
  string GetDbName();
  string GetUserId();
  string GetRimeVersion();

 protected:
  Db* db_;
};

/// A template to define a user db class based on an implementation of rime::Db.
template <class BaseDb>
class UserDbWrapper : public BaseDb {
 public:
  RIME_API UserDbWrapper(const string& db_name);

  virtual bool CreateMetadata() {
    return BaseDb::CreateMetadata() &&
        UserDbHelper(this).UpdateUserInfo();
  }
  virtual bool Backup(const string& snapshot_file) {
    return UserDbHelper::IsUniformFormat(snapshot_file) ?
        UserDbHelper(this).UniformBackup(snapshot_file) :
        BaseDb::Backup(snapshot_file);
  }
  virtual bool Restore(const string& snapshot_file) {
    return UserDbHelper::IsUniformFormat(snapshot_file) ?
        UserDbHelper(this).UniformRestore(snapshot_file) :
        BaseDb::Restore(snapshot_file);
  }
};

/// Implements a component that serves as a factory for a user db class.
template <class BaseDb>
class UserDbComponent : public UserDb::Component {
 public:
  Db* Create(const string& name) override {
    return new UserDbWrapper<BaseDb>(name + extension());
  }

  string extension() const override;
  string snapshot_extension() const override;
};

class UserDbMerger : public Sink {
 public:
  explicit UserDbMerger(Db* db);
  virtual ~UserDbMerger();

  virtual bool MetaPut(const string& key, const string& value);
  virtual bool Put(const string& key, const string& value);

  void CloseMerge();

 protected:
  Db* db_;
  TickCount our_tick_;
  TickCount their_tick_;
  TickCount max_tick_;
  int merged_entries_;
};

class UserDbImporter : public Sink {
 public:
  explicit UserDbImporter(Db* db);

  virtual bool MetaPut(const string& key, const string& value);
  virtual bool Put(const string& key, const string& value);

 protected:
  Db* db_;
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
