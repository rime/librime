//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-28 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TICKET_H_
#define RIME_TICKET_H_

#include <string>

namespace rime {

class Engine;
class Schema;

struct Ticket {
  Engine* engine;
  Schema* schema;
  std::string name_space;
  std::string klass;

  Ticket();
  Ticket(Schema* s, const std::string& ns);
  // prescription: in the form of "klass" or "klass@alias"
  // where alias, if given, will override default name space
  Ticket(Engine* e, const std::string& ns,
         const std::string& prescription = "");
};

}  // namespace rime

#endif  // RIME_TICKET_H_
