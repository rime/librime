// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/menu.h>

namespace rime {

Composition::Composition() {
}

const std::string Composition::GetText() const {
  std::string result;
  BOOST_FOREACH(const Segment &seg, *this) {
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    result += cand->text();
  }
  return result;
}

}  // namespace rime
