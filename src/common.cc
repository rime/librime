//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
//
// 2011-10-02 GONG Chen <chen.sst@gmail.com>
//

#include <boost/filesystem.hpp>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/service.h>

// built-in components
#include <rime/config.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/abc_segmentor.h>
#include <rime/gear/ascii_composer.h>
#include <rime/gear/ascii_segmentor.h>
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
#include <rime/gear/speller.h>
#include <rime/gear/reverse_lookup_translator.h>
#include <rime/gear/table_translator.h>
//#include <rime/gear/trivial_translator.h>
#include <rime/gear/uniquifier.h>

namespace rime {

void SetupLogging(const char* app_name) {
  google::InitGoogleLogging(app_name);
}

void RegisterComponents() {
  LOG(INFO) << "registering built-in components";

  Registry &r = Registry::instance();

  boost::filesystem::path user_data_dir =
      Service::instance().deployer().user_data_dir;
  boost::filesystem::path config_path = user_data_dir / "%s.yaml";
  boost::filesystem::path schema_path = user_data_dir / "%s.schema.yaml";
  r.Register("config", new ConfigComponent(config_path.string()));
  r.Register("schema_config", new ConfigComponent(schema_path.string()));

  r.Register("userdb", new Component<UserDb>);

  r.Register("dictionary", new DictionaryComponent);
  r.Register("reverse_lookup_dictionary", new ReverseLookupDictionaryComponent);
  r.Register("user_dictionary", new UserDictionaryComponent);

  // processors
  r.Register("ascii_composer", new Component<AsciiComposer>);
  r.Register("chord_composer", new Component<ChordComposer>);
  r.Register("express_editor", new Component<ExpressEditor>);
  r.Register("fluency_editor", new Component<FluencyEditor>);
  r.Register("key_binder", new Component<KeyBinder>);
  r.Register("navigator", new Component<Navigator>);
  r.Register("punctuator", new Component<Punctuator>);
  r.Register("recognizer", new Component<Recognizer>);
  r.Register("selector", new Component<Selector>);
  r.Register("speller", new Component<Speller>);

  // segmentors
  r.Register("abc_segmentor", new Component<AbcSegmentor>);
  r.Register("ascii_segmentor", new Component<AsciiSegmentor>);
  r.Register("matcher", new Component<Matcher>);
  r.Register("punct_segmentor", new Component<PunctSegmentor>);
  r.Register("fallback_segmentor", new Component<FallbackSegmentor>);

  // translators
  r.Register("echo_translator", new Component<EchoTranslator>);
  r.Register("punct_translator", new Component<PunctTranslator>);
  //r.Register("trivial_translator", new Component<TrivialTranslator>);
  r.Register("table_translator", new Component<TableTranslator>);
  r.Register("script_translator", new Component<ScriptTranslator>);
  r.Register("r10n_translator", new Component<ScriptTranslator>);  // alias
  r.Register("reverse_lookup_translator",
             new Component<ReverseLookupTranslator>);

  // filters
  r.Register("simplifier", new Component<Simplifier>);
  r.Register("uniquifier", new Component<Uniquifier>);
}

}  // namespace rime
