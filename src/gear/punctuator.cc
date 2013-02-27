// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-21 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <utf8.h>
#include <rime/commit_history.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/gear/punctuator.h>

namespace rime {

void PunctConfig::LoadConfig(Engine *engine, bool load_symbols) {
  bool full_shape = engine->context()->get_option("full_shape");
  std::string shape(full_shape ? "full_shape" : "half_shape");
  if (shape_ == shape) return;
  shape_ = shape;
  Config *config = engine->schema()->config();
  std::string preset;
  if (config->GetString("punctuator/import_preset", &preset)) {
    scoped_ptr<Config> preset_config(Config::Require("config")->Create(preset));
    if (!preset_config) {
      LOG(ERROR) << "Error importing preset punctuation '" << preset << "'.";
      return;
    }
    preset_mapping_ = preset_config->GetMap("punctuator/" + shape);
    if (!preset_mapping_) {
      LOG(WARNING) << "missing preset punctuation mapping.";
    }
    if (load_symbols && !preset_symbols_) {
      preset_symbols_ = preset_config->GetMap("punctuator/symbols");
    }
  }
  mapping_ = config->GetMap("punctuator/" + shape);
  if (!mapping_ && !preset_mapping_) {
    LOG(WARNING) << "missing punctuation mapping.";
  }
  if (load_symbols) {
    symbols_ = config->GetMap("punctuator/symbols");
  }
}

const ConfigItemPtr PunctConfig::GetPunctDefinition(const std::string key) {
  ConfigItemPtr punct_definition;
  if (mapping_)
    punct_definition = mapping_->Get(key);
  if (punct_definition)
    return punct_definition;
  
  if (preset_mapping_)
    punct_definition = preset_mapping_->Get(key);
  if (punct_definition)
    return punct_definition;

  if (symbols_)
    punct_definition = symbols_->Get(key);
  if (punct_definition)
    return punct_definition;
  
  if (preset_symbols_)
    punct_definition = preset_symbols_->Get(key);
  return punct_definition;
}

Punctuator::Punctuator(Engine *engine) : Processor(engine),
                                         use_space_(false),
                                         oddness_(0) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetBool("punctuator/use_space", &use_space_);
  }
  config_.LoadConfig(engine);
}

Processor::Result Punctuator::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt())
    return kNoop;
  int ch = key_event.keycode();
  if (ch < 0x20 || ch >= 0x7f)
    return kNoop;
  if (!use_space_ && ch == XK_space && engine_->context()->IsComposing())
    return kNoop;
  if (ch == '.' || ch == ':') {  // 3.14, 12:30
    const CommitHistory& history(engine_->context()->commit_history());
    if (!history.empty()) {
      const CommitRecord& cr(history.back());
      if (cr.type == "thru" && cr.text.length() == 1 && isdigit(cr.text[0])) {
        return kRejected;
      }
    }
  }
  config_.LoadConfig(engine_);
  std::string punct_key(1, ch);
  ConfigItemPtr punct_definition(config_.GetPunctDefinition(punct_key));
  if (!punct_definition)
    return kNoop;
  DLOG(INFO) << "punct key: '" << punct_key << "'";
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
      LOG(ERROR) << "missing candidate for punctuation '" << key << "'.";
      return false;
    }
    DLOG(INFO) << "alternating punctuation '" << key << "'.";
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
      LOG(ERROR) << "missing candidate for paired punctuation.";
      return false;
    }
    DLOG(INFO) << "alternating paired punctuation.";
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
  if (ch < 0x20 || ch >= 0x7f)
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
    DLOG(INFO) << "add a punctuation segment ["
               << segment.start << ", " << segment.end << ")";
    segment.tags.insert("punct");
    segmentation->AddSegment(segment);
  }
  return false;  // exclusive
}

PunctTranslator::PunctTranslator(const TranslatorTicket& ticket)
    : Translator(ticket) {
  const bool load_symbols = true;
  config_.LoadConfig(engine_, load_symbols);
}

shared_ptr<Candidate> CreatePunctCandidate(const std::string &punct, const Segment &segment) {
  const char half_shape[] = "\xe3\x80\x94\xe5\x8d\x8a\xe8\xa7\x92\xe3\x80\x95";  // 〔半角〕
  const char full_shape[] = "\xe3\x80\x94\xe5\x85\xa8\xe8\xa7\x92\xe3\x80\x95";  // 〔全角〕
  bool is_half_shape = false;
  bool is_full_shape = false;
  const char* p = punct.c_str();
  uint32_t ch = utf8::unchecked::next(p);
  if (*p == '\0') {  // length == 1 unicode character
    bool is_ascii = (ch >= 0x20 && ch < 0x7F);
    bool is_ideographic_space = (ch == 0x3000);
    bool is_full_shape_ascii = (ch >= 0xFF01 && ch <= 0xFF5E);
    bool is_half_shape_kana = (ch >= 0xFF65 && ch <= 0xFFDC);
    is_half_shape = is_ascii || is_half_shape_kana;
    is_full_shape = is_ideographic_space || is_full_shape_ascii;
  }
  bool one_key = (segment.end - segment.start == 1);
  return boost::make_shared<SimpleCandidate>("punct",
                                             segment.start,
                                             segment.end,
                                             punct,
                                             (is_half_shape ? half_shape :
                                              is_full_shape ? full_shape : ""),
                                             one_key ? punct : "");
}

shared_ptr<Translation> PunctTranslator::Query(const std::string &input,
                                               const Segment &segment,
                                               std::string* prompt) {
  if (!segment.HasTag("punct"))
    return shared_ptr<Translation>();
  config_.LoadConfig(engine_);
  ConfigItemPtr definition(config_.GetPunctDefinition(input));
  if (!definition)
    return shared_ptr<Translation>();
  DLOG(INFO) << "populating punctuation candidates for '" << input << "'.";
  shared_ptr<Translation> translation = TranslateUniquePunct(input, segment, As<ConfigValue>(definition));
  if (!translation)
    translation = TranslateAlternatingPunct(input, segment, As<ConfigList>(definition));
  if (!translation)
    translation = TranslateAutoCommitPunct(input, segment, As<ConfigMap>(definition));
  if (!translation)
    translation = TranslatePairedPunct(input, segment, As<ConfigMap>(definition));
  //if (prompt && translation) {
  //  const char tips[] = "\xe3\x80\x94\xe7\xac\xa6\xe8\x99\x9f\xe3\x80\x95";  // 〔符號〕
  //  *prompt = tips;
  //}
  return translation;
}

shared_ptr<Translation> PunctTranslator::TranslateUniquePunct(const std::string &key,
                                                              const Segment &segment,
                                                              const ConfigValuePtr &definition) {
  if (!definition)
    return shared_ptr<Translation>();
  return make_shared<UniqueTranslation>(CreatePunctCandidate(definition->str(), segment));
}

shared_ptr<Translation> PunctTranslator::TranslateAlternatingPunct(const std::string &key,
                                                                   const Segment &segment,
                                                                   const ConfigListPtr &definition) {
  if (!definition)
    return shared_ptr<Translation>();
  shared_ptr<FifoTranslation> translation(new FifoTranslation);
  for (size_t i = 0; i < definition->size(); ++i) {
    ConfigValuePtr value = definition->GetValueAt(i);
    if (!value) {
      LOG(WARNING) << "invalid alternating punct at index " << i << " for '" << key << "'.";
      continue;
    }
    translation->Append(CreatePunctCandidate(value->str(), segment));
  }
  if (!translation->size()) {
    LOG(WARNING) << "empty candidate list for alternating punct '" << key << "'.";
    translation.reset();
  }
  return translation;
}

shared_ptr<Translation> PunctTranslator::TranslateAutoCommitPunct(const std::string &key,
                                                                  const Segment &segment,
                                                                  const ConfigMapPtr &definition) {
  if (!definition || !definition->HasKey("commit"))
    return shared_ptr<Translation>();
  ConfigValuePtr value = definition->GetValue("commit");
  if (!value) {
    LOG(WARNING) << "unrecognized punct definition for '" << key << "'.";
    return shared_ptr<Translation>();
  }
  return make_shared<UniqueTranslation>(CreatePunctCandidate(value->str(), segment));
}

shared_ptr<Translation> PunctTranslator::TranslatePairedPunct(const std::string &key,
                                                              const Segment &segment,
                                                              const ConfigMapPtr &definition) {
  if (!definition || !definition->HasKey("pair"))
    return shared_ptr<Translation>();
  ConfigListPtr list = As<ConfigList>(definition->Get("pair"));
  if (!list || list->size() != 2) {
    LOG(WARNING) << "unrecognized pair definition for '" << key << "'.";
    return shared_ptr<Translation>();
  }
  shared_ptr<FifoTranslation> translation(new FifoTranslation);
  for (size_t i = 0; i < list->size(); ++i) {
    ConfigValuePtr value = list->GetValueAt(i);
    if (!value) {
      LOG(WARNING) << "invalid paired punct at index " << i << " for '" << key << "'.";
      continue;
    }
    translation->Append(CreatePunctCandidate(value->str(), segment));
  }
  if (translation->size() != 2) {
    LOG(WARNING) << "invalid num of candidate for paired punct '" << key << "'.";
    translation.reset();
  }
  return translation;
}

}  // namespace rime
