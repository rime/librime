// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#include <rime/context.h>
#include "trivial_translator.h"

namespace rime {

void TrivialTranslator::Query(Context *context,
                              Translation *translation) {
  // TODO:
  translation->set_result(context->input());
}

}  // namespace rime
