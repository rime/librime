//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/contextual_translation.h>

namespace rime {

using WordGraph = map<int, UserDictEntryCollector>;

class Grammar;
class Language;

class Poet {
 public:
  // sentence "less", used to compare sentences of the same input range.
  using Compare = function<bool (const Sentence&, const Sentence&)>;

  static bool CompareWeight(const Sentence& one, const Sentence& other) {
    return one.weight() < other.weight();
  }
  static bool LeftAssociateCompare(const Sentence& one, const Sentence& other);

  Poet(const Language* language, Config* config,
       Compare compare = CompareWeight);
  ~Poet();

  an<Sentence> MakeSentence(const WordGraph& graph,
                            size_t total_length,
                            const string& preceding_text);

  template <class TranslatorT>
  an<Translation> ContextualWeighted(an<Translation> translation,
                                     const string& input,
                                     size_t start,
                                     TranslatorT* translator) {
    if (!translator->contextual_suggestions() || !grammar_) {
      return translation;
    }
    auto preceding_text = translator->GetPrecedingText(start);
    if (preceding_text.empty()) {
      return translation;
    }
    return New<ContextualTranslation>(
        translation, input, preceding_text, grammar_.get());
  }

 private:
  const Language* language_;
  the<Grammar> grammar_;
  Compare compare_;
};

}  // namespace rime

#endif  // RIME_POET_H_
