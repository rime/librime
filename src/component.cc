// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// register components
//
// 2011-04-13 GONG Chen <chen.sst@gmail.com>
//

// TODO: include implementations of built-in components
#include <rime/registry.h>
#include <rime/config.h>
#include "abc_segmentor.h"
#include "trivial_processor.h"
#include "trivial_dictionary.h"

namespace rime {

void RegisterRimeComponents()
{
  EZLOGGERPRINT("registering built-in components");

  Registry &r = Registry::instance();
  r.Register("config", new ConfigComponent("%s.yaml"));
  r.Register("schema_config", new ConfigComponent("%s.schema.yaml"));

  // processors
  r.Register("trivial_processor", new Component<TrivialProcessor>);

  // segmentors
  r.Register("abc_segmentor", new Component<AbcSegmentor>);
  
  // dictionaries
  r.Register("trivial_dictionary", new Component<TrivialDictionary>);

}

}  // namespace rime
