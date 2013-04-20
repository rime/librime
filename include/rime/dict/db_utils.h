//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DB_UTILS_H_
#define RIME_DB_UTILS_H_

#include <string>
#include <rime/common.h>

namespace rime {

class Sink {
 public:
  virtual ~Sink() {}
  virtual bool MetaPut(const std::string& key, const std::string& value) = 0;
  virtual bool Put(const std::string& key, const std::string& value) = 0;

  template <class SourceType>
  int operator<< (SourceType& source);
};

class Source {
 public:
  virtual ~Source() {}
  virtual bool MetaGet(std::string* key, std::string* value) = 0;
  virtual bool Get(std::string* key, std::string* value) = 0;

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

  virtual bool MetaPut(const std::string& key, const std::string& value);
  virtual bool Put(const std::string& key, const std::string& value);

 protected:
  Db* db_;
};

class DbSource : public Source {
 public:
  explicit DbSource(Db* db);

  virtual bool MetaGet(std::string* key, std::string* value);
  virtual bool Get(std::string* key, std::string* value);

 protected:
  Db* db_;
  shared_ptr<DbAccessor> metadata_;
  shared_ptr<DbAccessor> data_;
};

}  // namespace rime

#endif  // RIME_DB_UTILS_H_
