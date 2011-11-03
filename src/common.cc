// vim: set sts=2 sw=2 et:
// encoding: utf-8
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

// TODO: include headers for built-in components
#include <rime/config.h>
#include <rime/impl/abc_segmentor.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/echo_translator.h>
#include <rime/impl/express_editor.h>
#include <rime/impl/fluency_editor.h>
#include <rime/impl/fallback_segmentor.h>
#include <rime/impl/r10n_translator.h>
#include <rime/impl/selector.h>
#include <rime/impl/speller.h>
#include <rime/impl/table_translator.h>
#include <rime/impl/trivial_translator.h>
#include <rime/impl/user_dictionary.h>

namespace rime {

const std::string GetLogFilePath() {
  boost::filesystem::path dir(ConfigDataManager::instance().user_data_dir());
  boost::filesystem::create_directories(dir);
  return (dir / "rime.log").string();
}

void RegisterComponents() {
  EZLOGGERPRINT("registering built-in components");

  Registry &r = Registry::instance();
  
  boost::filesystem::path user_data_dir(ConfigDataManager::instance().user_data_dir());
  boost::filesystem::path config_path = user_data_dir / "%s.yaml";
  boost::filesystem::path schema_path = user_data_dir / "%s.schema.yaml";
  r.Register("config", new ConfigComponent(config_path.string()));
  r.Register("schema_config", new ConfigComponent(schema_path.string()));
  
  r.Register("dictionary", new DictionaryComponent);
  r.Register("user_dictionary", new UserDictionaryComponent);

  // processors
  r.Register("express_editor", new Component<ExpressEditor>);
  r.Register("fluency_editor", new Component<FluencyEditor>);
  r.Register("selector", new Component<Selector>);
  r.Register("speller", new Component<Speller>);

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
