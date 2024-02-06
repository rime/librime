//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TSV_H_
#define RIME_TSV_H_

#include <rime/common.h>

namespace rime {

using Tsv = vector<string>;

using TsvParser = function<bool(const Tsv& row, string* key, string* value)>;

using TsvFormatter =
    function<bool(const string& key, const string& value, Tsv* row)>;

class Sink;
class Source;

class TsvReader {
 public:
  TsvReader(const path& file_path, TsvParser parser)
      : file_path_(file_path), parser_(parser) {}
  // return number of records read
  int operator()(Sink* sink);

 protected:
  path file_path_;
  TsvParser parser_;
};

class TsvWriter {
 public:
  TsvWriter(const path& file_path, TsvFormatter formatter)
      : file_path_(file_path), formatter_(formatter) {}
  // return number of records written
  int operator()(Source* source);

 protected:
  path file_path_;
  TsvFormatter formatter_;

 public:
  string file_description;
};

template <class SinkType>
int operator<<(SinkType& sink, TsvReader& reader) {
  return reader(&sink);
}

template <class SinkType>
int operator>>(TsvReader& reader, SinkType& sink) {
  return reader(&sink);
}

template <class SourceType>
int operator<<(TsvWriter& writer, SourceType& source) {
  return writer(&source);
}

template <class SourceType>
int operator>>(SourceType& source, TsvWriter& writer) {
  return writer(&source);
}

}  // namespace rime

#endif  // RIME_TSV_H_
