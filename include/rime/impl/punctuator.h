// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-21 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_PUNCTUATOR_H_
#define RIME_PUNCTUATOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/config.h>
#include <rime/processor.h>
#include <rime/segmentor.h>
#include <rime/translator.h>

namespace rime {

class PunctConfig {
 public:
  void LoadConfig(Engine *engine);
  const ConfigItemPtr GetPunctDefinition(const std::string key);
 protected:
  ConfigMapPtr mapping_;
  ConfigMapPtr preset_mapping_;
  std::string shape_;
};

class Punctuator : public Processor {
 public:
  Punctuator(Engine *engine);
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);

 protected:
  bool ConfirmUniquePunct(const ConfigItemPtr &definition);
  bool AlternatePunct(const std::string &key, const ConfigItemPtr &definition);
  bool AutoCommitPunct(const ConfigItemPtr &definition);
  bool PairPunct(const ConfigItemPtr &definition);

  PunctConfig config_;
  int oddness_;
};

class PunctSegmentor : public Segmentor {
 public:
  PunctSegmentor(Engine *engine);
  virtual bool Proceed(Segmentation *segmentation);

 protected:
  PunctConfig config_;
};

class PunctTranslator : public Translator {
 public:
  PunctTranslator(Engine *engine);
  virtual Translation* Query(const std::string &input, const Segment &segment);

 protected:
  Translation* TranslateUniquePunct(const std::string &key,
                                    const Segment &segment,
                                    const ConfigValuePtr &definition);
  Translation* TranslateAlternatingPunct(const std::string &key,
                                         const Segment &segment,
                                         const ConfigListPtr &definition);
  Translation* TranslateAutoCommitPunct(const std::string &key,
                                        const Segment &segment,
                                        const ConfigMapPtr &definition);
  Translation* TranslatePairedPunct(const std::string &key,
                                    const Segment &segment,
                                    const ConfigMapPtr &definition);

  PunctConfig config_;
};

}  // namespace rime

#endif  // RIME_PUNCTUATOR_H_
