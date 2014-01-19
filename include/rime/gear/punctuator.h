//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-11-21 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_PUNCTUATOR_H_
#define RIME_PUNCTUATOR_H_

#include <map>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/config.h>
#include <rime/processor.h>
#include <rime/segmentor.h>
#include <rime/translator.h>

namespace rime {

class Engine;

class PunctConfig {
 public:
  void LoadConfig(Engine *engine, bool load_symbols = false);
  ConfigItemPtr GetPunctDefinition(const std::string key);
 protected:
  ConfigMapPtr mapping_;
  ConfigMapPtr preset_mapping_;
  std::string shape_;
  ConfigMapPtr symbols_;
  ConfigMapPtr preset_symbols_;
};

class Punctuator : public Processor {
 public:
  Punctuator(const Ticket& ticket);
  virtual ProcessResult ProcessKeyEvent(const KeyEvent &key_event);

 protected:
  bool ConfirmUniquePunct(const ConfigItemPtr &definition);
  bool AlternatePunct(const std::string &key, const ConfigItemPtr &definition);
  bool AutoCommitPunct(const ConfigItemPtr &definition);
  bool PairPunct(const ConfigItemPtr &definition);

  PunctConfig config_;
  bool use_space_;
  std::map<ConfigItemPtr, int> oddness_;
};

class PunctSegmentor : public Segmentor {
 public:
  PunctSegmentor(const Ticket& ticket);
  virtual bool Proceed(Segmentation *segmentation);

 protected:
  PunctConfig config_;
};

class PunctTranslator : public Translator {
 public:
  PunctTranslator(const Ticket& ticket);
  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);

 protected:
  shared_ptr<Translation>
  TranslateUniquePunct(const std::string &key,
                       const Segment &segment,
                       const ConfigValuePtr &definition);
  shared_ptr<Translation>
  TranslateAlternatingPunct(const std::string &key,
                            const Segment &segment,
                            const ConfigListPtr &definition);
  shared_ptr<Translation>
  TranslateAutoCommitPunct(const std::string &key,
                           const Segment &segment,
                           const ConfigMapPtr &definition);
  shared_ptr<Translation>
  TranslatePairedPunct(const std::string &key,
                       const Segment &segment,
                       const ConfigMapPtr &definition);

  PunctConfig config_;
};

}  // namespace rime

#endif  // RIME_PUNCTUATOR_H_
