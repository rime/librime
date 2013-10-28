//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-02-27 GONG Chen <chen.sst@gmail.com>
//
#include <rime/ticket.h>
#include <rime/translator.h>

namespace rime {

Translator::Translator(const Ticket& ticket)
    : engine_(ticket.engine), name_space_(ticket.name_space) {
}


}  // namespace rime
