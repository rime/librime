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
  an<ConfigItem> GetPunctDefinition(const string key);
 protected:
  string shape_;
  an<ConfigMap> mapping_;
  an<ConfigMap> symbols_;
};

class Punctuator : public Processor {
 public:
  Punctuator(const Ticket& ticket);
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  bool ConfirmUniquePunct(const an<ConfigItem>& definition);
  bool AlternatePunct(const string& key, const an<ConfigItem>& definition);
  bool AutoCommitPunct(const an<ConfigItem>& definition);
  bool PairPunct(const an<ConfigItem>& definition);

  PunctConfig config_;
  bool use_space_ = false;
  map<an<ConfigItem>, int> oddness_;
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
  virtual an<Translation> Query(const string& input,
                                        const Segment& segment);

 protected:
  an<Translation>
  TranslateUniquePunct(const string& key,
                       const Segment& segment,
                       const an<ConfigValue>& definition);
  an<Translation>
  TranslateAlternatingPunct(const string& key,
                            const Segment& segment,
                            const an<ConfigList>& definition);
  an<Translation>
  TranslateAutoCommitPunct(const string& key,
                           const Segment& segment,
                           const an<ConfigMap>& definition);
  an<Translation>
  TranslatePairedPunct(const string& key,
                       const Segment& segment,
                       const an<ConfigMap>& definition);

  PunctConfig config_;
};

}  // namespace rime

#endif  // RIME_PUNCTUATOR_H_
