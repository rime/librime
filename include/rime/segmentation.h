// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SEGMENTATION_H_
#define RIME_SEGMENTATION_H_

#include <string>
#include <vector>

namespace rime {

struct Segment {
  int start;
  int end;
  std::vector<std::string> tags;
};

class Segmentation {
 public:
  Segmentation(const std::string &input);

  bool Add(const Segment &segment);
  bool Forward();
  bool HasFinished() const;

  const std::string& input() const { return input_; }
  const std::vector<Segment> segments() const { return segments_; }

 private:
  const std::string input_;
  std::vector<Segment> segments_;
  int cursor_;
};

}  // namespace rime

#endif  // RIME_SEGMENTATION_H_
