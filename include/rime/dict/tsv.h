//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TSV_H_
#define RIME_TSV_H_

#include <string>
#include <vector>
#include <boost/function.hpp>

namespace rime {

typedef std::vector<std::string> Tsv;

typedef boost::function<bool (const Tsv& row,
                              std::string* key,
                              std::string* value)> TsvParser;

typedef boost::function<bool (const std::string& key,
                              const std::string& value,
                              Tsv* row)> TsvFormatter;

class TsvSink {
 public:
  virtual ~TsvSink() {}
  virtual bool MetaPut(const std::string& key, const std::string& value) = 0;
  virtual bool Put(const std::string& key, const std::string& value) = 0;
};

class TsvSource {
 public:
  virtual ~TsvSource() {}
  virtual bool MetaGet(std::string* key, std::string* value) = 0;
  virtual bool Get(std::string* key, std::string* value) = 0;
  std::string file_description;
};

class TsvReader {
 public:
  explicit TsvReader(TsvSink* sink, TsvParser parser)
      : sink_(sink), parser_(parser) {
  }
  // return number of records read
  int operator() (const std::string& path);
 protected:
  TsvSink* sink_;  // weak ref
  TsvParser parser_;
};

class TsvWriter {
 public:
  TsvWriter(TsvSource* source, TsvFormatter formatter)
      : source_(source), formatter_(formatter) {
  }
  // return number of records written
  int operator() (const std::string& path);
 protected:
  TsvSource* source_;  // weak ref
  TsvFormatter formatter_;
};

}  // namespace rime

#endif  // RIME_TSV_H_
