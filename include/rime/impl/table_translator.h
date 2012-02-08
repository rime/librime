// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TABLE_TRANSLATOR_H_
#define RIME_TABLE_TRANSLATOR_H_

#include <string>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/dict/dictionary.h>

namespace rime {

class TableTranslation : public Translation {
 public:
  TableTranslation(const std::string& input, size_t start, size_t end,
                   const std::string& preedit,
                   Projection* comment_formatter);
  TableTranslation(const DictEntryIterator& iter,
                   const std::string& input, size_t start, size_t end,
                   const std::string& preedit,
                   Projection* comment_formatter);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();
  
 protected:
  DictEntryIterator iter_;
  std::string input_;
  size_t start_;
  size_t end_;
  std::string preedit_;
  Projection *comment_formatter_;
};

class TableTranslator : public Translator {
 public:
  TableTranslator(Engine *engine);
  virtual ~TableTranslator();

  virtual Translation* Query(const std::string &input,
                             const Segment &segment);

 protected:
  scoped_ptr<Dictionary> dict_;
  bool enable_completion_;
  Projection preedit_formatter_;
  Projection comment_formatter_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
