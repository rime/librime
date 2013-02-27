//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-02-27 GONG Chen <chen.sst@gmail.com>
//
#include <rime/translator.h>

namespace rime {

TranslatorTicket::TranslatorTicket(Engine* an_engine,
                                   const std::string& instruction)
    : engine(an_engine), klass(instruction) {
  size_t separator = klass.find('@');
  if (separator != std::string::npos) {
    alias = klass.substr(separator + 1);
    klass.resize(separator);
  }
}

Translator::Translator(const TranslatorTicket& ticket)
    : engine_(ticket.engine),
      name_space_(ticket.alias.empty()? "translator" : ticket.alias) {
}


}  // namespace rime
