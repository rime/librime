//
// Copyright RIME Developers
// Distributed under the BSD License
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

class Engine;

class PunctConfig {
 public:
  void LoadConfig(Engine* engine, bool load_symbols = false);
  a<ConfigItem> GetPunctDefinition(const string key);
 protected:
  a<ConfigMap> mapping_;
  a<ConfigMap> preset_mapping_;
  string shape_;
  a<ConfigMap> symbols_;
  a<ConfigMap> preset_symbols_;
};

class Punctuator : public Processor {
 public:
  Punctuator(const Ticket& ticket);
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  bool ConfirmUniquePunct(const a<ConfigItem>& definition);
  bool AlternatePunct(const string& key, const a<ConfigItem>& definition);
  bool AutoCommitPunct(const a<ConfigItem>& definition);
  bool PairPunct(const a<ConfigItem>& definition);

  PunctConfig config_;
  bool use_space_ = false;
  map<a<ConfigItem>, int> oddness_;
};

class PunctSegmentor : public Segmentor {
 public:
  PunctSegmentor(const Ticket& ticket);
  virtual bool Proceed(Segmentation* segmentation);

 protected:
  PunctConfig config_;
};

class PunctTranslator : public Translator {
 public:
  PunctTranslator(const Ticket& ticket);
  virtual a<Translation> Query(const string& input,
                                        const Segment& segment);

 protected:
  a<Translation>
  TranslateUniquePunct(const string& key,
                       const Segment& segment,
                       const a<ConfigValue>& definition);
  a<Translation>
  TranslateAlternatingPunct(const string& key,
                            const Segment& segment,
                            const a<ConfigList>& definition);
  a<Translation>
  TranslateAutoCommitPunct(const string& key,
                           const Segment& segment,
                           const a<ConfigMap>& definition);
  a<Translation>
  TranslatePairedPunct(const string& key,
                       const Segment& segment,
                       const a<ConfigMap>& definition);

  PunctConfig config_;
};

}  // namespace rime

#endif  // RIME_PUNCTUATOR_H_
