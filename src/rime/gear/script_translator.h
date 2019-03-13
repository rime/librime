//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCRIPT_TRANSLATOR_H_
#define RIME_SCRIPT_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/gear/memory.h>
#include <rime/gear/translator_commons.h>

namespace rime {

class Code;
class Corrector;
struct DictEntry;
struct DictEntryCollector;
class Dictionary;
class Poet;
class UserDictionary;
struct SyllableGraph;

class ScriptTranslator : public Translator,
                         public Memory,
                         public TranslatorOptions {
 public:
  ScriptTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                const Segment& segment);
  virtual bool Memorize(const CommitEntry& commit_entry);

  string FormatPreedit(const string& preedit);
  string Spell(const Code& code);

  // options
  int max_homophones() const { return max_homophones_; }
  int spelling_hints() const { return spelling_hints_; }
  bool always_show_comments() const { return always_show_comments_; }

 protected:
  int max_homophones_ = 1;
  int spelling_hints_ = 0;
  bool always_show_comments_ = false;
  bool enable_correction_ = false;
  the<Corrector> corrector_;
  the<Poet> poet_;
};

}  // namespace rime

#endif  // RIME_SCRIPT_TRANSLATOR_H_
