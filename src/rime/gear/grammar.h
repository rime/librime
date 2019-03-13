#ifndef RIME_GRAMMAR_H_
#define RIME_GRAMMAR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/vocabulary.h>

namespace rime {

class Config;

class Grammar : public Class<Grammar, Config*> {
 public:
  virtual ~Grammar() {}
  virtual double Query(const string& context,
                       const string& word,
                       bool is_rear) = 0;

  inline static double Evaluate(const string& context,
                                const DictEntry& entry,
                                bool is_rear,
                                Grammar* grammar) {
    const double kPenalty = -18.420680743952367; // log(1e-8)
    return entry.weight +
        (grammar ? grammar->Query(context, entry.text, is_rear) : kPenalty);
  }
};

}  // namespace rime

#endif  // RIME_GRAMMAR_H_
