// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-21 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/impl/punctuator.h>

namespace rime {

void PunctConfig::LoadConfig(Engine *engine) {
  bool full_shape = engine->context()->get_option("full_shape");
  std::string shape(full_shape ? "full_shape" : "half_shape");
  if (shape_ == shape) return;
  shape_ = shape;
  Config *config = engine->schema()->config();
  std::string preset;
  if (config->GetString("punctuator/import_preset", &preset)) {
    scoped_ptr<Config> preset_config(Config::Require("config")->Create(preset));
    if (!preset_config) {
      EZLOGGERPRINT("Error importing preset punctuation '%s'.", preset.c_str());
      return;
    }
    preset_mapping_ = preset_config->GetMap("punctuator/" + shape);
    if (!preset_mapping_) {
      EZLOGGERPRINT("Warning: missing preset punctuation mapping.");
    }
  }
  mapping_ = config->GetMap("punctuator/" + shape);
  if (!mapping_ && !preset_mapping_) {
    EZLOGGERPRINT("Warning: missing punctuation mapping.");
  }
}

const ConfigItemPtr PunctConfig::GetPunctDefinition(const std::string key) {
  ConfigItemPtr punct_definition;
  if (mapping_)
    punct_definition = mapping_->Get(key);
  if (!punct_definition && preset_mapping_)
    punct_definition = preset_mapping_->Get(key);
  return punct_definition;
}

Punctuator::Punctuator(Engine *engine) : Processor(engine), oddness_(0) {
  config_.LoadConfig(engine);
}

Processor::Result Punctuator::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt())
    return kNoop;
  int ch = key_event.keycode();
  if (ch < 0x20 || ch > 0x7f)
    return kNoop;
  if (ch == XK_space && engine_->context()->IsComposing())
    return kNoop;
  config_.LoadConfig(engine_);
  std::string punct_key(1, ch);
  ConfigItemPtr punct_definition(config_.GetPunctDefinition(punct_key));
  if (!punct_definition)
    return kNoop;
  EZDBGONLYLOGGERVAR(punct_key);
  if (!AlternatePunct(punct_key, punct_definition)) {
    engine_->context()->PushInput(ch) &&
        (ConfirmUniquePunct(punct_definition) ||
         AutoCommitPunct(punct_definition) ||
         PairPunct(punct_definition));
  }
  return kAccepted;
}

bool Punctuator::AlternatePunct(const std::string &key,
                                const ConfigItemPtr &definition) {
  if (!As<ConfigList>(definition))
    return false;
  Context *ctx = engine_->context();
  Composition *comp = ctx->composition();
  if (comp->empty())
    return false;
  Segment &segment(comp->back());
  if (segment.status > Segment::kVoid && segment.HasTag("punct") &&
      key == ctx->input().substr(segment.start, segment.end - segment.start)) {
    if (!segment.menu || segment.menu->Prepare(segment.selected_index + 2) == 0) {
      EZLOGGERPRINT("Error: missing candidate for punctuation '%s'.", key.c_str());
      return false;
    }
    EZLOGGERPRINT("Info: alternating punctuation '%s'.", key.c_str());
    (segment.selected_index += 1) %= segment.menu->candidate_count();
    segment.status = Segment::kGuess;
    return true;
  }
  return false;
}

bool Punctuator::ConfirmUniquePunct(const ConfigItemPtr &definition) {
  if (!As<ConfigValue>(definition))
    return false;
  Context *ctx = engine_->context();
  ctx->ConfirmCurrentSelection();
  return true;
}

bool Punctuator::AutoCommitPunct(const ConfigItemPtr &definition) {
  ConfigMapPtr map(As<ConfigMap>(definition));
  if (!map || !map->HasKey("commit"))
    return false;
  Context *ctx = engine_->context();
  ctx->Commit();
  return true;
}

bool Punctuator::PairPunct(const ConfigItemPtr &definition) {
  ConfigMapPtr map(As<ConfigMap>(definition));
  if (!map || !map->HasKey("pair"))
    return false;
  Context *ctx = engine_->context();
  Composition *comp = ctx->composition();
  if (comp->empty())
    return false;
  Segment &segment(comp->back());
  if (segment.status > Segment::kVoid && segment.HasTag("punct")) {
    if (!segment.menu || segment.menu->Prepare(2) < 2) {
      EZLOGGERPRINT("Error: missing candidate for pared punctuation.");
      return false;
    }
    EZLOGGERPRINT("Info: alternating paired punctuation.");
    (segment.selected_index += oddness_) %= 2;
    oddness_ = 1 - oddness_;
    ctx->ConfirmCurrentSelection();
    return true;
  }
  return false;
}

PunctSegmentor::PunctSegmentor(Engine *engine) : Segmentor(engine) {
  config_.LoadConfig(engine);
}

bool PunctSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input = segmentation->input();
  int k = segmentation->GetCurrentStartPosition();
  if (k == input.length())
    return false;  // no chance for others too
  char ch = input[k];
  if (ch < 0x20 || ch > 0x7f)
    return true;
  config_.LoadConfig(engine_);
  std::string punct_key(1, ch);
  ConfigItemPtr punct_definition(config_.GetPunctDefinition(punct_key));
  if (!punct_definition)
    return true;
  {
    Segment segment;
    segment.start = k;
    segment.end = k + 1;
    EZDBGONLYLOGGERPRINT("add a punctuation segment [%d, %d)",
                         segment.start, segment.end);
    segment.tags.insert("punct");
    segmentation->AddSegment(segment);
  }
  return false;  // exclusive
}

PunctTranslator::PunctTranslator(Engine *engine) : Translator(engine) {
  config_.LoadConfig(engine);
}

shared_ptr<Candidate> CreatePunctCandidate(const std::string &punct, const Segment &segment) {
  bool is_ascii = (punct.length() == 1 && punct[0] >= 0x20 && punct[0] <= 0x7f);
  const char half_shape[] = "\xe3\x80\x94\xe5\x8d\x8a\xe8\xa7\x92\xe3\x80\x95";  // 〔半角〕
  return shared_ptr<Candidate>(new SimpleCandidate(
      "punct", segment.start, segment.end, punct, (is_ascii ? half_shape : ""), punct));
}

Translation* PunctTranslator::Query(const std::string &input, const Segment &segment) {
  if (!segment.HasTag("punct"))
    return NULL;
  config_.LoadConfig(engine_);
  ConfigItemPtr definition(config_.GetPunctDefinition(input));
  if (!definition)
    return NULL;
  EZLOGGERPRINT("Info: populating punctuation candidates for '%s'.", input.c_str());
  Translation *translation = TranslateUniquePunct(input, segment, As<ConfigValue>(definition));
  if (!translation)
    translation = TranslateAlternatingPunct(input, segment, As<ConfigList>(definition));
  if (!translation)
    translation = TranslateAutoCommitPunct(input, segment, As<ConfigMap>(definition));
  if (!translation)
    translation = TranslatePairedPunct(input, segment, As<ConfigMap>(definition));
  return translation;
}

Translation* PunctTranslator::TranslateUniquePunct(const std::string &key,
                                                   const Segment &segment,
                                                   const ConfigValuePtr &definition) {
  if (!definition)
    return NULL;
  return new UniqueTranslation(CreatePunctCandidate(definition->str(), segment));
}

Translation* PunctTranslator::TranslateAlternatingPunct(const std::string &key,
                                                        const Segment &segment,
                                                        const ConfigListPtr &definition) {
  if (!definition)
    return NULL;
  FifoTranslation *translation = new FifoTranslation;
  for (size_t i = 0; i < definition->size(); ++i) {
    ConfigValuePtr value = definition->GetValueAt(i);
    if (!value) {
      EZLOGGERPRINT("Warning: invalid alternating punct at index %d for '%s'.", i, key.c_str());
      continue;
    }
    translation->Append(CreatePunctCandidate(value->str(), segment));
  }
  if (!translation->size()) {
    EZLOGGERPRINT("Warning: empty candidate list for alternating punct '%s'.", key.c_str());
    delete translation;
    return NULL;
  }
  return translation;
}

Translation* PunctTranslator::TranslateAutoCommitPunct(const std::string &key,
                                                       const Segment &segment,
                                                       const ConfigMapPtr &definition) {
  if (!definition || !definition->HasKey("commit"))
    return NULL;
  ConfigValuePtr value = definition->GetValue("commit");
  if (!value) {
    EZLOGGERPRINT("Warning: unrecognized punct definition for '%s'.", key.c_str());
    return NULL;
  }
  return new UniqueTranslation(CreatePunctCandidate(value->str(), segment));
}

Translation* PunctTranslator::TranslatePairedPunct(const std::string &key,
                                                   const Segment &segment,
                                                   const ConfigMapPtr &definition) {
  if (!definition || !definition->HasKey("pair"))
    return NULL;
  ConfigListPtr list = As<ConfigList>(definition->Get("pair"));
  if (!list || list->size() != 2) {
    EZLOGGERPRINT("Warning: unrecognized pair definition for '%s'.", key.c_str());
    return NULL;
  }
  FifoTranslation *translation = new FifoTranslation;
  for (size_t i = 0; i < list->size(); ++i) {
    ConfigValuePtr value = list->GetValueAt(i);
    if (!value) {
      EZLOGGERPRINT("Warning: invalid paired punct at index %d for '%s'.", i, key.c_str());
      continue;
    }
    translation->Append(CreatePunctCandidate(value->str(), segment));
  }
  if (translation->size() != 2) {
    EZLOGGERPRINT("Warning: invalid num of candidate for paired punct '%s'.", key.c_str());
    delete translation;
    return NULL;
  }
  return translation;
}

}  // namespace rime
