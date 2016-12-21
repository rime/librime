//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TABLE_DB_H_
#define RIME_TABLE_DB_H_

#include <rime/dict/text_db.h>

namespace rime {

class TableDb : public TextDb {
 public:
  explicit TableDb(const string& name);

  static const TextFormat format;
};

// read-only tabledb
class StableDb : public TableDb {
 public:
  explicit StableDb(const string& name);

  virtual bool Open();
};

}  // namespace rime

#endif  // RIME_TEXT_DB_H_
