//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <rime/common.h>
#include <rime/dict/db_utils.h>
#include <rime/dict/tsv.h>

namespace rime {

int TsvReader::operator() (Sink* sink) {
  if (!sink) return 0;
  LOG(INFO) << "reading tsv file: " << path_;
  std::ifstream fin(path_.c_str());
  string line, key, value;
  Tsv row;
  int line_no = 0;
  int num_entries = 0;
  bool enable_comment = true;
  while (getline(fin, line)) {
    ++line_no;
    boost::algorithm::trim_right(line);
    // skip empty lines and comments
    if (line.empty()) continue;
    if (enable_comment && line[0] == '#') {
      if (boost::starts_with(line, "#@")) {
        // metadata
        line.erase(0, 2);
        boost::algorithm::split(row, line,
                                boost::algorithm::is_any_of("\t"));
        if (row.size() != 2 ||
            !sink->MetaPut(row[0], row[1])) {
          LOG(WARNING) << "invalid metadata at line " << line_no << ".";
        }
      }
      else if (line == "# no comment") {
        // a "# no comment" line disables further comments
        enable_comment = false;
      }
      continue;
    }
    // read a tsv entry
    boost::algorithm::split(row, line,
                            boost::algorithm::is_any_of("\t"));
    if (!parser_(row, &key, &value) ||
        !sink->Put(key, value)) {
      LOG(WARNING) << "invalid entry at line " << line_no << ".";
      continue;
    }
    ++num_entries;
  }
  fin.close();
  return num_entries;
}

int TsvWriter::operator() (Source* source) {
  if (!source) return 0;
  LOG(INFO) << "writing tsv file: " << path_;
  std::ofstream fout(path_.c_str());
  if (!file_description.empty()) {
    fout << "# " << file_description << std::endl;
  }
  string key, value;
  while (source->MetaGet(&key, &value)) {
    fout << "#@" << key << '\t' << value << std::endl;
  }
  Tsv row;
  int num_entries = 0;
  while (source->Get(&key, &value)) {
    row.clear();
    if (formatter_(key, value, &row) && !row.empty()) {
      for (auto it = row.cbegin(); it != row.cend(); ++it) {
        if (it != row.cbegin())
          fout << '\t';
        fout << *it;
      }
      fout << std::endl;
      ++num_entries;
    }
  }
  fout.close();
  return num_entries;
}

}  // namespace rime
