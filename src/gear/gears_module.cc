//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>

#include <rime/gear/abc_segmentor.h>
#include <rime/gear/affix_segmentor.h>
#include <rime/gear/ascii_composer.h>
#include <rime/gear/ascii_segmentor.h>
#include <rime/gear/charset_filter.h>
#include <rime/gear/chord_composer.h>
#include <rime/gear/echo_translator.h>
#include <rime/gear/editor.h>
#include <rime/gear/fallback_segmentor.h>
#include <rime/gear/matcher.h>
#include <rime/gear/navigator.h>
#include <rime/gear/key_binder.h>
#include <rime/gear/punctuator.h>
#include <rime/gear/script_translator.h>
#include <rime/gear/recognizer.h>
#include <rime/gear/selector.h>
#include <rime/gear/simplifier.h>
#include <rime/gear/single_char_filter.h>
#include <rime/gear/shape.h>
#include <rime/gear/speller.h>
#include <rime/gear/reverse_lookup_filter.h>
#include <rime/gear/reverse_lookup_translator.h>
#include <rime/gear/schema_list_translator.h>
#include <rime/gear/switch_translator.h>
#include <rime/gear/table_translator.h>
#include <rime/gear/uniquifier.h>
#include <rime/gear/codepoint_translator.h>
#include <rime/gear/history_translator.h>

static void rime_gears_initialize() {
  using namespace rime;

  LOG(INFO) << "registering components from module 'gears'.";
  Registry& r = Registry::instance();

  // processors
  r.Register("ascii_composer", new Component<AsciiComposer>);
  r.Register("chord_composer", new Component<ChordComposer>);
  r.Register("express_editor", new Component<ExpressEditor>);
  r.Register("fluid_editor", new Component<FluidEditor>);
  r.Register("fluency_editor", new Component<FluidEditor>);  // alias
  r.Register("key_binder", new Component<KeyBinder>);
  r.Register("navigator", new Component<Navigator>);
  r.Register("punctuator", new Component<Punctuator>);
  r.Register("recognizer", new Component<Recognizer>);
  r.Register("selector", new Component<Selector>);
  r.Register("speller", new Component<Speller>);
  r.Register("shape_processor", new Component<ShapeProcessor>);

  // segmentors
  r.Register("abc_segmentor", new Component<AbcSegmentor>);
  r.Register("affix_segmentor", new Component<AffixSegmentor>);
  r.Register("ascii_segmentor", new Component<AsciiSegmentor>);
  r.Register("matcher", new Component<Matcher>);
  r.Register("punct_segmentor", new Component<PunctSegmentor>);
  r.Register("fallback_segmentor", new Component<FallbackSegmentor>);

  // translators
  r.Register("echo_translator", new Component<EchoTranslator>);
  r.Register("punct_translator", new Component<PunctTranslator>);
  r.Register("table_translator", new Component<TableTranslator>);
  r.Register("script_translator", new Component<ScriptTranslator>);
  r.Register("r10n_translator", new Component<ScriptTranslator>);  // alias
  r.Register("reverse_lookup_translator",
             new Component<ReverseLookupTranslator>);
  r.Register("schema_list_translator", new Component<SchemaListTranslator>);
  r.Register("switch_translator", new Component<SwitchTranslator>);
  r.Register("codepoint_translator", new Component<CodepointTranslator>);
  r.Register("history_translator", new Component<HistoryTranslator>);

  // filters
  r.Register("simplifier", new Component<Simplifier>);
  r.Register("uniquifier", new Component<Uniquifier>);
  r.Register("charset_filter", new Component<CharsetFilter>);
  r.Register("cjk_minifier", new Component<CharsetFilter>);  // alias
  r.Register("reverse_lookup_filter", new Component<ReverseLookupFilter>);
  r.Register("single_char_filter", new Component<SingleCharFilter>);

  // formatters
  r.Register("shape_formatter", new Component<ShapeFormatter>);
}

static void rime_gears_finalize() {
}

RIME_REGISTER_MODULE(gears)
