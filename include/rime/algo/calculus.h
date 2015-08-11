//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_CALCULUS_H_
#define RIME_CALCULUS_H_

#include <stdint.h>
#include <boost/regex.hpp>
#include <rime/common.h>
#include "spelling.h"

namespace rime {

class Calculation {
 public:
  using Factory = Calculation* (const vector<string>& args);

  Calculation() = default;
  virtual ~Calculation() = default;
  virtual bool Apply(Spelling* spelling) = 0;
  virtual bool addition() { return true; }
  virtual bool deletion() { return true; }
};

class Calculus {
 public:
  Calculus();
  void Register(const string& token, Calculation::Factory* factory);
  Calculation* Parse(const string& defintion);

 private:
  map<string, Calculation::Factory*> factories_;
};

// xlit/zyx/abc/
class Transliteration : public Calculation {
 public:
  static Factory Parse;
  bool Apply(Spelling* spelling);

 protected:
  map<uint32_t, uint32_t> char_map_;
};

// xform/x/y/
class Transformation : public Calculation {
 public:
  static Factory Parse;
  bool Apply(Spelling* spelling);

 protected:
  boost::regex pattern_;
  string replacement_;
};

// erase/x/
class Erasion : public Calculation {
 public:
  static Factory Parse;
  bool Apply(Spelling* spelling);
  bool addition() { return false; }

 protected:
  boost::regex pattern_;
};

// derive/x/X/
class Derivation : public Transformation {
 public:
  static Factory Parse;
  bool deletion() { return false; }
};

// fuzz/zyx/zx/
class Fuzzing : public Derivation {
 public:
  static Factory Parse;
  bool Apply(Spelling* spelling);
};

// abbrev/zyx/z/
class Abbreviation : public Derivation {
 public:
  static Factory Parse;
  bool Apply(Spelling* spelling);
};

}  // namespace rime

#endif  // RIME_CALCULUS_H_
