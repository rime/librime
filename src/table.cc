// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-02 GONG Chen <chen.sst@gmail.com>
//

#include <rime/table.h>

namespace rime {

bool Table::Load() {
  return false;
}

bool Table::Save() {
  return false;
}

void Table::Build(/* arguements to argue */) {
  
}

const TableEntryVector* Table::GetEntries(int syllable_id) {
  return NULL;
}

}  // namespace rime
