//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-11-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_FILTER_COMMONS_H_
#define RIME_FILTER_COMMONS_H_


namespace rime {

struct Segment;
struct Ticket;

class TagMatching {
 public:
  explicit TagMatching(const Ticket& ticket);
  bool TagsMatch(Segment* segment);

 protected:
  vector<string> tags_;
};

}  // namespace rime

#endif  // RIME_FILTER_COMMONS_H_
