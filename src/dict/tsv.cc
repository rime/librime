//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-14 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <rime/common.h>
#include <rime/dict/tsv.h>

namespace rime {

int TsvReader::operator() (const std::string& path) {
  LOG(INFO) << "reading tsv file: " << path;
  std::ifstream fin(path.c_str());
  std::string line, key, value;
  Tsv row;
  int line_no = 0;
  int num_entries = 0;
  bool enable_comment = true;
  while (getline(fin, line)) {
    ++line_no;
    // skip empty lines and comments
    if (line.empty()) continue;
    if (enable_comment && line[0] == '#') {
      if (boost::starts_with(line, "#@")) {
        // metadata
        line.erase(0, 2);
        boost::algorithm::split(row, line,
                                boost::algorithm::is_any_of("\t"));
        if (row.size() != 2 ||
            !sink_->MetaPut(row[0], row[1])) {
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
        !sink_->Put(key, value)) {
      LOG(WARNING) << "invalid entry at line " << line_no << ".";
      continue;
    }
    ++num_entries;
  }
  fin.close();
  return num_entries;
}

int TsvWriter::operator() (const std::string& path) {
  LOG(INFO) << "writing tsv file: " << path;
  std::ofstream fout(path.c_str());
  if (!source_->file_description.empty()) {
    fout << "# " << source_->file_description << std::endl;
  }
  std::string key, value;
  while (source_->MetaGet(&key, &value)) {
    fout << "#@" << key << '\t' << value << std::endl;
  }
  Tsv row;
  int num_entries = 0;
  while (source_->Get(&key, &value)) {
    row.clear();
    if (formatter_(key, value, &row) && !row.empty()) {
      for (Tsv::const_iterator it = row.begin(); it != row.end(); ++it) {
        if (it != row.begin())
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
