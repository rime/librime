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

static void load_patterns(RecognizerPatterns *patterns, ConfigMapPtr map) {
  if (!patterns || !map) return;
  ConfigMap::Iterator it = map->begin();
  for (; it != map->end(); ++it) {
    ConfigValuePtr value = As<ConfigValue>(it->second);
    if (!value) continue;
    (*patterns)[it->first] = value->str();
  }
}

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
    load_patterns(this, pattern_map);
  }
  pattern_map = config->GetMap("recognizer/patterns");
  load_patterns(this, pattern_map);
}

RecognizerMatch RecognizerPatterns::GetMatch(
    const std::string &input, Segmentation *segmentation) const {
  size_t j = segmentation->GetCurrentEndPosition();
  size_t k = segmentation->GetConfirmedPosition();
  std::string active_input(input.substr(k));
  EZLOGGER(k, active_input);
  bool found = false;
  BOOST_FOREACH(const RecognizerPatterns::value_type &v, *this) {
    boost::smatch m;
    if (boost::regex_search(active_input, m, v.second)) {
      size_t start = k + m.position();
      size_t end = start + m.length();
      if (end != input.length()) continue;
      if (start == j) {
        EZLOGGERPRINT("input[%d,%d] '%s' matches pattern: %s",
                      start, end, m.str().c_str(), v.first.c_str());
        return RecognizerMatch(v.first, start, end);
      }
      BOOST_FOREACH(const Segment &seg, *segmentation) {
        if (start < seg.start) break;
        if (start == seg.start) {
          EZLOGGERPRINT("input[%d,%d] '%s' matches pattern: %s",
                        start, end, m.str().c_str(), v.first.c_str());
          return RecognizerMatch(v.first, start, end);
        }
      }
    }
  }
  return RecognizerMatch();
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
  int ch = key_event.keycode();
  if (ch > 0x20 && ch < 0x80) {
    // pattern matching against the input string plus the incoming character
    Context *ctx = engine_->context();
    std::string input(ctx->input());
    input += ch;
    RecognizerMatch m = patterns_.GetMatch(input, ctx->composition());
    if (m.found()) {
      ctx->PushInput(ch);
      return kAccepted;
    }
  }
  return kNoop;
}

}  // namespace rime
