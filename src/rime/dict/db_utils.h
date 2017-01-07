//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DB_UTILS_H_
#define RIME_DB_UTILS_H_

#include <rime/common.h>

namespace rime {

class Sink {
 public:
  virtual ~Sink() = default;
  virtual bool MetaPut(const string& key, const string& value) = 0;
  virtual bool Put(const string& key, const string& value) = 0;

  template <class SourceType>
  int operator<< (SourceType& source);
};

class Source {
 public:
  virtual ~Source() = default;
  virtual bool MetaGet(string* key, string* value) = 0;
  virtual bool Get(string* key, string* value) = 0;

  template <class SinkType>
  int operator>> (SinkType& sink);

  int Dump(Sink* sink);
};

template <class SourceType>
int Sink::operator<< (SourceType& source) {
  return source.Dump(this);
}

template <class SinkType>
int Source::operator>> (SinkType& sink) {
  return Dump(&sink);
}

class Db;
class DbAccessor;

class DbSink : public Sink {
 public:
  explicit DbSink(Db* db);

  virtual bool MetaPut(const string& key, const string& value);
  virtual bool Put(const string& key, const string& value);

 protected:
  Db* db_;
};

class DbSource : public Source {
 public:
  explicit DbSource(Db* db);

  virtual bool MetaGet(string* key, string* value);
  virtual bool Get(string* key, string* value);

 protected:
  Db* db_;
  an<DbAccessor> metadata_;
  an<DbAccessor> data_;
};

}  // namespace rime

#endif  // RIME_DB_UTILS_H_
