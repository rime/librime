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

#include <set>
#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

class Candidate;
class Menu;

struct Segment {
  enum Status {
    kVoid,
    kGuess,
    kSelected,
    kConfirmed,
  };
  Status status;
  size_t start;
  size_t end;
  std::set<std::string> tags;
  shared_ptr<Menu> menu;
  size_t selected_index;

  Segment()
  : status(kVoid), start(0), end(0),
    selected_index(0) {}

  Segment(int start_pos, int end_pos)
  : status(kVoid), start(start_pos), end(end_pos),
    selected_index(0) {}

  void Clear() {
    status = kVoid;
    tags.clear();
    menu.reset();
    selected_index = 0;
  }

  bool HasTag(const std::string &tag) const {
    return tags.find(tag) != tags.end();
  }

  const shared_ptr<Candidate> GetCandidateAt(size_t index) const;
  const shared_ptr<Candidate> GetSelectedCandidate() const;
};

class Segmentation : public std::vector<Segment> {
 public:
  Segmentation();
  virtual ~Segmentation() {}
  void Reset(const std::string &input);
  void Reset(size_t num_segments);
  bool AddSegment(const Segment &segment);

  bool Forward();
  bool HasFinishedSegmentation() const;
  size_t GetCurrentStartPosition() const;
  size_t GetCurrentEndPosition() const;
  size_t GetCurrentSegmentLength() const;

  const std::string& input() const { return input_; }

 protected:
  std::string input_;
};

std::ostream& operator<< (std::ostream& out,
                          const Segmentation &segmentation);

}  // namespace rime

#endif  // RIME_SEGMENTATION_H_
