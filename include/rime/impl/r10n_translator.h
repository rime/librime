// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_R10N_TRANSLATOR_H_
#define RIME_R10N_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

struct DictEntry;
class DictEntryCollector;
class Dictionary;
struct SyllableGraph;

class R10nTranslator : public Translator {
 public:
  R10nTranslator(Engine *engine);
  virtual ~R10nTranslator();

  virtual Translation* Query(const std::string &input,
                             const Segment &segment);
  
 protected:
  shared_ptr<DictEntry> SimplisticSentenceMaking(const SyllableGraph& syllable_graph);

 private:
  scoped_ptr<Dictionary> dict_;
};

}  // namespace rime

#endif  // RIME_R10N_TRANSLATOR_H_
