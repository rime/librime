//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATOR_H_
#define RIME_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/ticket.h>

namespace rime {

class Context;
class Dictionary;
class Engine;
class Translation;
struct Segment;

//! Tab entry for input disambiguation
struct InputTabEntry {
  enum Source { kSyllable, kTableCode, kReverseLookup, kRawInput };
  std::string label;
  size_t span;
  int source;  // one of Source enum values
};

class Translator : public Class<Translator, const Ticket&> {
 public:
  explicit Translator(const Ticket& ticket)
      : engine_(ticket.engine), name_space_(ticket.name_space) {}
  virtual ~Translator() = default;

  virtual an<Translation> Query(const string& input,
                                const Segment& segment) = 0;

  //! Override to collect tab entries for disambiguation at a position.
  virtual void CollectInputTabs(size_t position,
                                vector<InputTabEntry>* tabs) const {}
  //! Override to return the primary dictionary for code lookup.
  virtual Dictionary* primary_dictionary() const { return nullptr; }
  //! Override to return the reverse lookup prefix, if any.
  virtual string reverse_lookup_prefix() const { return ""; }

  string name_space() const { return name_space_; }

 protected:
  Engine* engine_;
  string name_space_;
};

}  // namespace rime

#endif  // RIME_TRANSLATOR_H_
