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

#include <boost/signals/connection.hpp>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

struct DictEntry;
struct DictEntryCollector;
class Dictionary;
class UserDictionary;
struct SyllableGraph;

class R10nTranslator : public Translator {
 public:
  R10nTranslator(Engine *engine);
  virtual ~R10nTranslator();

  virtual Translation* Query(const std::string &input,
                             const Segment &segment);

  // options
  const std::string& delimiters() const { return delimiters_; }
  bool enable_completion() const { return enable_completion_; }
  
 protected:
  void OnCommit(Context *ctx);
  
  scoped_ptr<Dictionary> dict_;
  scoped_ptr<UserDictionary> user_dict_;
  std::string delimiters_;
  bool enable_completion_;
  boost::signals::connection connection_;
};

}  // namespace rime

#endif  // RIME_R10N_TRANSLATOR_H_
