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
#include <rime/impl/abc_segmentor.h>
#include <rime/impl/echo_translator.h>
#include <rime/impl/fallback_segmentor.h>
#include <rime/impl/r10n_translator.h>
#include <rime/impl/selector.h>
#include <rime/impl/table_translator.h>
#include <rime/impl/trivial_processor.h>
#include <rime/impl/trivial_translator.h>

namespace rime {

void RegisterComponents()
{
  EZLOGGERPRINT("registering built-in components");

  Registry &r = Registry::instance();
  r.Register("config", new ConfigComponent("%s.yaml"));
  r.Register("schema_config", new ConfigComponent("%s.schema.yaml"));

  // processors
  r.Register("trivial_processor", new Component<TrivialProcessor>);
  r.Register("selector", new Component<Selector>);

  // segmentors
  r.Register("abc_segmentor", new Component<AbcSegmentor>);
  r.Register("fallback_segmentor", new Component<FallbackSegmentor>);

  // translators
  r.Register("echo_translator", new Component<EchoTranslator>);
  r.Register("trivial_translator", new Component<TrivialTranslator>);
  r.Register("table_translator", new Component<TableTranslator>);
  r.Register("r10n_translator", new Component<R10nTranslator>);

}

}  // namespace rime
