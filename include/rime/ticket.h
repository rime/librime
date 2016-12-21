//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-28 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TICKET_H_
#define RIME_TICKET_H_


namespace rime {

class Engine;
class Schema;

struct Ticket {
  Engine* engine = nullptr;
  Schema* schema = nullptr;
  string name_space;
  string klass;

  Ticket() = default;
  Ticket(Schema* s, const string& ns);
  // prescription: in the form of "klass" or "klass@alias"
  // where alias, if given, will override default name space
  Ticket(Engine* e, const string& ns = "",
         const string& prescription = "");
};

}  // namespace rime

#endif  // RIME_TICKET_H_
