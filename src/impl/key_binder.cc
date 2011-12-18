// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-23 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/impl/key_binder.h>

namespace rime {

struct KeyBinding {
  std::string whence;
  KeyEvent pattern;
  KeyEvent target;
};

class KeyBindings : public std::vector<KeyBinding> {
 public:
  void LoadConfig(Schema *schema);
 protected:
  void LoadBindings(const ConfigListPtr &bindings);
};



void KeyBindings::LoadConfig(Schema *schema) {
  if (!schema) return;
  Config *config = schema->config();
  ConfigListPtr bindings = config->GetList("key_binder/bindings");
  if (bindings)
    LoadBindings(bindings);
  std::string preset;
  if (config->GetString("key_binder/import_preset", &preset)) {
    scoped_ptr<Config> preset_config(Config::Require("config")->Create(preset));
    if (!preset_config) {
      EZLOGGERPRINT("Error importing preset key bindings '%s'.", preset.c_str());
      return;
    }
    bindings = preset_config->GetList("key_binder/bindings");
    if (bindings)
      LoadBindings(bindings);
    else
      EZLOGGERPRINT("Warning: missing preset key bindings.");
  }
}

void KeyBindings::LoadBindings(const ConfigListPtr &bindings) {
  if (!bindings) return;
  for (size_t i = 0; i < bindings->size(); ++i) {
    ConfigMapPtr map = As<ConfigMap>(bindings->GetAt(i));
    if (!map) continue;
    ConfigValuePtr whence = map->GetValue("when");
    if (!whence) continue;
    ConfigValuePtr pattern = map->GetValue("accept");
    if (!pattern) continue;
    ConfigValuePtr target = map->GetValue("send");
    if (!target) continue;
    KeyBinding binding;
    binding.whence = whence->str();
    if (!binding.pattern.Parse(pattern->str()) ||
        !binding.target.Parse(target->str())) {
      EZLOGGERPRINT("Warning: invalid key binding #%d.", i);
      continue;
    }
    push_back(binding);
  }
}

KeyBinder::KeyBinder(Engine *engine) : Processor(engine),
                                       key_bindings_(new KeyBindings),
                                       redirecting_(false) {
  key_bindings_->LoadConfig(engine->schema());
}

typedef std::set<std::string> Conditions;

static void measure_conditions(Engine *engine, Conditions *conditions) {
  Context *ctx = engine->context();
  if (ctx->IsComposing()) {
    conditions->insert("composing");
  }
  if (ctx->HasMenu() && !engine->get_option("ascii_mode")) {
    conditions->insert("has_menu");
  }
  Composition *comp = ctx->composition();
  if (!comp->empty() && comp->back().HasTag("paging")) {
    conditions->insert("paging");
  }
}

Processor::Result KeyBinder::ProcessKeyEvent(const KeyEvent &key_event) {
  if (redirecting_ || !key_bindings_ || key_bindings_->empty())
    return kNoop;
  Conditions conditions;
  bool first_match = true;
  BOOST_FOREACH(const KeyBinding &binding, *key_bindings_) {
    if (key_event == binding.pattern) {
      if (first_match) {
        measure_conditions(engine_, &conditions);
        first_match = false;
      }
      if (conditions.find(binding.whence) != conditions.end()) {
        redirecting_ = true;
        engine_->ProcessKeyEvent(binding.target);
        redirecting_ = false;
        return kAccepted;
      }
    }
  }
  // not handled
  return kNoop;
}

}  // namespace rime
