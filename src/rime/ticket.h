//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-28 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TICKET_H_
#define RIME_TICKET_H_

#include <rime_api.h>
#include <rime/engine.h>

namespace rime {

struct Ticket {
  Engine* engine = nullptr;
  Schema* schema = nullptr;
  string name_space;
  string klass;

  Ticket() = default;
  Ticket(Schema* s, string_view ns);
  // prescription: in the form of "klass" or "klass@alias"
  // where alias, if given, will override default name space
  RIME_API Ticket(Engine* e,
                  string_view ns = ""sv,
                  string_view prescription = ""sv);
};

}  // namespace rime

#endif  // RIME_TICKET_H_
