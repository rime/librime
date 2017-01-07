//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/gear/recognizer.h>

namespace rime {

static void load_patterns(RecognizerPatterns* patterns, an<ConfigMap> map) {
  if (!patterns || !map)
    return;
  for (auto it = map->begin(); it != map->end(); ++it) {
    auto value = As<ConfigValue>(it->second);
    if (!value)
      continue;
    (*patterns)[it->first] = value->str();
  }
}

void RecognizerPatterns::LoadConfig(Config* config) {
  an<ConfigMap> pattern_map;
  string preset;
  if (config->GetString("recognizer/import_preset", &preset)) {
    the<Config> preset_config(
        Config::Require("config")->Create(preset));
    if (!preset_config) {
      LOG(ERROR) << "Error importing preset patterns '" << preset << "'.";
      return;
    }
    pattern_map = preset_config->GetMap("recognizer/patterns");
    load_patterns(this, pattern_map);
  }
  pattern_map = config->GetMap("recognizer/patterns");
  load_patterns(this, pattern_map);
}

RecognizerMatch
RecognizerPatterns::GetMatch(const string& input,
                             const Segmentation& segmentation) const {
  size_t j = segmentation.GetCurrentEndPosition();
  size_t k = segmentation.GetConfirmedPosition();
  string active_input = input.substr(k);
  DLOG(INFO) << "matching active input '" << active_input << "' at pos " << k;
  for (const auto& v : *this) {
    boost::smatch m;
    if (boost::regex_search(active_input, m, v.second)) {
      size_t start = k + m.position();
      size_t end = start + m.length();
      if (end != input.length())
        continue;
      if (start == j) {
        DLOG(INFO) << "input [" << start << ", " << end << ") '"
                   << m.str() << "' matches pattern: " << v.first;
        return {v.first, start, end};
      }
      for (const Segment& seg : segmentation) {
        if (start < seg.start)
          break;
        if (start == seg.start) {
          DLOG(INFO) << "input [" << start << ", " << end << ") '"
                     << m.str() << "' matches pattern: " << v.first;
          return {v.first, start, end};
        }
      }
    }
  }
  return RecognizerMatch();
}

Recognizer::Recognizer(const Ticket& ticket) : Processor(ticket) {
  if (!ticket.schema)
    return;
  if (Config* config = ticket.schema->config()) {
    patterns_.LoadConfig(config);
    config->GetBool("recognizer/use_space", &use_space_);
  }
}

ProcessResult Recognizer::ProcessKeyEvent(const KeyEvent& key_event) {
  if (patterns_.empty() ||
      key_event.ctrl() || key_event.alt() || key_event.release()) {
    return kNoop;
  }
  int ch = key_event.keycode();
  if ((use_space_ && ch == ' ') ||
      (ch > 0x20 && ch < 0x80)) {
    // pattern matching against the input string plus the incoming character
    Context* ctx = engine_->context();
    string input = ctx->input();
    input += ch;
    auto match = patterns_.GetMatch(input, ctx->composition());
    if (match.found()) {
      ctx->PushInput(ch);
      return kAccepted;
    }
  }
  return kNoop;
}

}  // namespace rime
