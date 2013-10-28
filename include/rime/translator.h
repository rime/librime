//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATOR_H_
#define RIME_TRANSLATOR_H_

#include <string>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class Context;
class Engine;
struct Segment;
struct Ticket;
class Translation;

class Translator : public Class<Translator, const Ticket&> {
 public:
  Translator(const Ticket& ticket);
  virtual ~Translator() {}

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt = NULL) = 0;

 protected:
  Engine *engine_;
  std::string name_space_;
};

}  // namespace rime

#endif  // RIME_TRANSLATOR_H_
