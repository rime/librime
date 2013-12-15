//
// Copyleft RIME Developers
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
  size_t start = 0, end = 0;

  RecognizerMatch() = default;
  RecognizerMatch(const std::string& a_tag, size_t a_start, size_t an_end)
      : tag(a_tag), start(a_start), end(an_end) {}

  bool found() const { return start < end; }
};

class RecognizerPatterns : public std::map<std::string, boost::regex> {
 public:
  void LoadConfig(Config* config);
  RecognizerMatch GetMatch(const std::string& input,
                           Segmentation* segmentation) const;
};

class Recognizer : public Processor {
 public:
  Recognizer(const Ticket& ticket);

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  RecognizerPatterns patterns_;
};

}  // namespace rime

#endif  // RIME_RECOGNIZER_H_
