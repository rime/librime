// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/impl/recognizer.h>

namespace rime {

void RecognizerPatterns::LoadConfig(Config *config) {
  ConfigMapPtr pattern_map;
  std::string preset;
  if (config->GetString("recognizer/import_preset", &preset)) {
    scoped_ptr<Config> preset_config(Config::Require("config")->Create(preset));
    if (!preset_config) {
      EZLOGGERPRINT("Error importing preset patterns '%s'.", preset.c_str());
      return;
    }
    pattern_map = preset_config->GetMap("recognizer/patterns");
  }
  else {
    pattern_map = config->GetMap("recognizer/patterns");
  }
  if (!pattern_map) return;
  ConfigMap::Iterator it = pattern_map->begin();
  for (; it != pattern_map->end(); ++it) {
    ConfigValuePtr value = As<ConfigValue>(it->second);
    if (!value) continue;
    (*this)[it->first] = value->str();
  }
}

Recognizer::Recognizer(Engine *engine) : Processor(engine) {
  Config *config = engine->schema()->config();
  if (!config) return;
  patterns_.LoadConfig(config);
}

Processor::Result Recognizer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (patterns_.empty() ||
      key_event.ctrl() || key_event.alt() || key_event.release()) {
    return kNoop;
  }
  Context *ctx = engine_->context();
  std::string input(ctx->input());
  int ch = key_event.keycode();
  if (ch > 0x20 && ch < 0x80) {
    // pattern matching against the input string plus the incoming character
    input += ch;
    BOOST_FOREACH(const RecognizerPatterns::value_type &v, patterns_) {
      if (boost::regex_match(input, v.second)) {
        EZLOGGERPRINT("recognized pattern: %s", v.first.c_str());
        ctx->PushInput(ch);
        return kAccepted;
      }
    }
  }
  return kNoop;
}

}  // namespace rime
