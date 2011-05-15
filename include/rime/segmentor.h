// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SEGMENTOR_H_
#define RIME_SEGMENTOR_H_

#include <rime/component.h>

namespace rime {

class Engine;

struct Segment {
  int start;
  int end;
  std::vector<std::string> tags;
};

class Segmentation {
 public:
  Segmentation(const std::string &input);

  bool Add(const Segment &segment);

  const std::string& input() const { return input_; }
  int cursor() const { return cursor_; }
  const std::vector<Segment> segments() const { return segments_; }

 private:
  const std::string &input_;
  int cursor_;
  std::vector<Segment> segments_;
};

class Segmentor : public Class<Segmentor, Engine*> {
 public:
  explicit Segmentor(Engine *engine) : engine_(engine) {}
  virtual ~Segmentor() {}

  virtual bool Proceed(Segmentation *segmentation) = 0;

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_SEGMENTOR_H_
