//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/dict/table_db.h>
#include <rime/dict/user_db.h>

// Rime table entry format:
// phrase <Tab> code [ <Tab> weight ]
// for multi-syllable phrase, code is a space-separated list of syllables
// weight is a double precision float, defaulting to 0.0

namespace rime {

static bool rime_table_entry_parser(const Tsv& row,
                                    std::string* key,
                                    std::string* value) {
  if (row.size() < 2 ||
      row[0].empty() || row[1].empty()) {
    return false;
  }
  std::string code(row[1]);
  boost::algorithm::trim(code);
  *key = code + " \t" + row[0];
  UserDbValue v;
  if (row.size() >= 3 && !row[2].empty()) {
    try {
      v.commits = boost::lexical_cast<int>(row[2]);
      const double kS = 1e8;
      v.dee = (v.commits + 1) / kS;
    }
    catch (...) {
    }
  }
  *value = v.Pack();
  return true;
}

static bool rime_table_entry_formatter(const std::string& key,
                                       const std::string& value,
                                       Tsv* tsv) {
  Tsv& row(*tsv);
  // key ::= code <space> <Tab> phrase
  boost::algorithm::split(row, key,
                          boost::algorithm::is_any_of("\t"));
  if (row.size() != 2 ||
      row[0].empty() || row[1].empty())
    return false;
  UserDbValue v(value);
  if (v.commits < 0)  // deleted entry
    return false;
  boost::algorithm::trim(row[0]);  // remove trailing space
  row[0].swap(row[1]);
  row.push_back(boost::lexical_cast<std::string>(v.commits));
  return true;
}

const TextFormat TableDb::format = {
  rime_table_entry_parser,
  rime_table_entry_formatter,
  "Rime table",
};

TableDb::TableDb(const std::string& name)
    : TextDb(name + ".txt", "tabledb", TableDb::format) {
}

}  // namespace rime
