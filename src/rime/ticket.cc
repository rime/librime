//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-28 GONG Chen <chen.sst@gmail.com>
//
#include <rime/ticket.h>

namespace rime {

Ticket::Ticket(Schema* s, string_view ns) : schema(s), name_space(ns) {}

Ticket::Ticket(Engine* e, string_view ns, string_view prescription)
    : engine(e),
      schema(e ? e->schema() : NULL),
      name_space(ns),
      klass(prescription) {
  size_t separator = klass.find('@');
  if (separator != string::npos) {
    name_space = klass.substr(separator + 1);
    klass.resize(separator);
  }
}

}  // namespace rime
