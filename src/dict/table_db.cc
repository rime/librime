//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#include <rime/dict/table_db.h>

namespace rime {

static bool rime_table_entry_parser(const Tsv& row,
                                    std::string* key,
                                    std::string* value) {
  // TODO:
  return false;
}

static bool rime_table_entry_formatter(const std::string& key,
                                       const std::string& value,
                                       Tsv* row) {
  // TODO:
  return false;
}

static TextFormat tabledb_format = {
  rime_table_entry_parser,
  rime_table_entry_formatter,
  "Rime table",
};

TableDb::TableDb(const std::string& name)
    : TextDb(name + ".txt", "tabledb", tabledb_format) {
}

}  // namespace rime
