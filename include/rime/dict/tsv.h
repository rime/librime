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

class Sink;
class Source;

class TsvReader {
 public:
  TsvReader(const std::string& path, TsvParser parser)
      : path_(path), parser_(parser) {
  }
  // return number of records read
  int operator() (Sink* sink);
 protected:
  std::string path_;
  TsvParser parser_;
};

class TsvWriter {
 public:
  TsvWriter(const std::string& path, TsvFormatter formatter)
      : path_(path), formatter_(formatter) {
  }
  // return number of records written
  int operator() (Source* source);
 protected:
  std::string path_;
  TsvFormatter formatter_;
 public:
  std::string file_description;
};

template <class SinkType>
int operator<< (SinkType& sink, TsvReader& reader) {
  return reader(&sink);
}

template <class SinkType>
int operator>> (TsvReader& reader, SinkType& sink) {
  return reader(&sink);
}

template <class SourceType>
int operator<< (TsvWriter& writer, SourceType& source) {
  return writer(&source);
}

template <class SourceType>
int operator>> (SourceType& source, TsvWriter& writer) {
  return writer(&source);
}

}  // namespace rime

#endif  // RIME_TSV_H_
