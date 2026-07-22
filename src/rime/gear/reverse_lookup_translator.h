//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_TRANSLATOR_H_
#define RIME_REVERSE_LOOKUP_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/algo/syllabifier.h>

namespace rime {

class Dictionary;
class ReverseLookupDictionary;
class TranslatorOptions;

class ReverseLookupTranslator : public Translator {
 public:
  ReverseLookupTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input, const Segment& segment);

  //! Get the configured reverse lookup prefix (e.g. "z")
  //! Triggers lazy initialization if not yet done.
  RIME_DLL string reverse_lookup_prefix() const {
    if (!initialized_)
      const_cast<ReverseLookupTranslator*>(this)->Initialize();
    return prefix_;
  }

  //! Return the primary dictionary for code lookup (override).
  Dictionary* primary_dictionary() const override { return dict(); }

  //! Collect possible tab entries from the reverse lookup SyllableGraph
  RIME_DLL void CollectReverseLookupTabs(size_t start_pos,
                                         vector<InputTabEntry>* tabs) const;

  void CollectInputTabs(size_t position,
                        vector<InputTabEntry>* tabs) const override {
    CollectReverseLookupTabs(position, tabs);
  }

 protected:
  void Initialize();

  string tag_;
  bool initialized_ = false;
  the<Dictionary> dict_;
  the<ReverseLookupDictionary> rev_dict_;
  the<TranslatorOptions> options_;
  string prefix_;
  string suffix_;
  string tips_;

  // expose dict() for virtual override
  Dictionary* dict() const {
    if (!initialized_)
      const_cast<ReverseLookupTranslator*>(this)->Initialize();
    return dict_.get();
  }
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_TRANSLATOR_H_
