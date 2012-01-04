// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_RECOGNIZER_H_
#define RIME_RECOGNIZER_H_

#include <map>
#include <string>
#include <boost/regex.hpp>
#include <rime/common.h>
#include <rime/processor.h>

namespace rime {

class Config;
class Segmentation;

struct RecognizerMatch {
  std::string tag;
  size_t start, end;

  RecognizerMatch() : tag(), start(0), end(0) {}
  RecognizerMatch(const std::string &_tag, size_t _start, size_t _end)
      : tag(_tag), start(_start), end(_end) {}
  
  bool found() const { return start < end; }
};

class RecognizerPatterns : public std::map<std::string, boost::regex> {
 public:
  void LoadConfig(Config *config);
  RecognizerMatch GetMatch(const std::string &input,
                           Segmentation *segmentation) const;
};

class Recognizer : public Processor {
 public:
  Recognizer(Engine *engine);
  
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
  
 protected:
  RecognizerPatterns patterns_;
};

}  // namespace rime

#endif  // RIME_RECOGNIZER_H_
