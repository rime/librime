// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#include <rime/context.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include "trivial_translator.h"

namespace rime {

Translation* TrivialTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  EZLOGGERPRINT("input = %s, [%d, %d)",
                input.c_str(), segment.start, segment.end);
  Translation *translation = new Translation;
  if (!translation)
    return NULL;
  
  translation->set_result(input);
  return translation;
}

}  // namespace rime
