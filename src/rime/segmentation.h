//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SEGMENTATION_H_
#define RIME_SEGMENTATION_H_

#include <rime_api.h>
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
  Status status = kVoid;
  size_t start = 0;
  size_t end = 0;
  size_t length = 0;
  set<string> tags;
  an<Menu> menu;
  size_t selected_index = 0;
  string prompt;

  Segment() = default;

  Segment(int start_pos, int end_pos)
      : start(start_pos), end(end_pos), length(end_pos - start_pos) {
  }

  void Clear() {
    status = kVoid;
    tags.clear();
    menu.reset();
    selected_index = 0;
    prompt.clear();
  }

  void Close();
  bool Reopen(size_t caret_pos);

  bool HasTag(const string& tag) const {
    return tags.find(tag) != tags.end();
  }

  an<Candidate> GetCandidateAt(size_t index) const;
  an<Candidate> GetSelectedCandidate() const;
};

class Segmentation : public vector<Segment> {
 public:
  RIME_API Segmentation();
  virtual ~Segmentation() {}
  RIME_API void Reset(const string& input);
  void Reset(size_t num_segments);
  bool AddSegment(Segment segment);

  bool Forward();
  bool Trim();
  RIME_API bool HasFinishedSegmentation() const;
  size_t GetCurrentStartPosition() const;
  size_t GetCurrentEndPosition() const;
  size_t GetCurrentSegmentLength() const;
  size_t GetConfirmedPosition() const;

  const string& input() const { return input_; }

 protected:
  string input_;
};

std::ostream& operator<< (std::ostream& out,
                          const Segmentation& segmentation);

}  // namespace rime

#endif  // RIME_SEGMENTATION_H_
