// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATION_H_
#define RIME_TRANSLATION_H_

#include <string>
#include <rime/common.h>

namespace rime {

class Candidate;

class Translation {
 public:
  Translation() {}
  virtual ~Translation() {}

  // A translation may contain multiple results, looks
  // something like a generator of candidates.
  virtual shared_ptr<Candidate> Next() = 0;

  virtual shared_ptr<const Candidate> Peek() const = 0;

  // should it provide the next candidate or
  // should it give the chance to other translations?
  virtual bool Precedes(const Translation *other) const { return true; }
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
