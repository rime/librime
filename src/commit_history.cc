// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-27 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/commit_history.h>
#include <rime/composition.h>
#include <rime/key_event.h>

namespace rime {

void CommitHistory::Push(const CommitRecord& record) {
  push_back(record);
  if (!empty() && size() > kMaxRecords)
    pop_front();
}

void CommitHistory::Push(const KeyEvent& key_event) {
  if (key_event.modifier() == 0) {
    if (key_event.keycode() == XK_BackSpace ||
        key_event.keycode() == XK_Return) {
      clear();
    }
    else if (key_event.keycode() >= 0x20 && key_event.keycode() <= 0x7e) {
      // printable ascii character
      Push(CommitRecord(key_event.keycode()));
    }
  }
}

void CommitHistory::Push(const Composition& composition,
                         const std::string& input) {
  CommitRecord* last = NULL;
  size_t end = 0;
  BOOST_FOREACH(const Segment &seg, composition) {
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    if (cand) {
      if (last && last->type == cand->type()) {
        // join adjacent text of same type
        last->text += cand->text();
      }
      else {
        // new record
        Push(CommitRecord(cand->type(), cand->text()));
        last = &back();
      }
      if (seg.status >= Segment::kConfirmed) {
        // terminate a record by confirmation
        last = NULL;
      }
      end = cand->end();
    }
    else {
      // no translation for the segment
      Push(CommitRecord("raw", input.substr(seg.start, seg.end - seg.start)));
      end = seg.end;
    }
  }
  if (input.length() > end) {
    Push(CommitRecord("raw", input.substr(end)));
  }
}

const std::string CommitHistory::repr() const {
  std::string result;
  BOOST_FOREACH(const CommitRecord& record, *this) {
    result += "[" + record.type + "]" + record.text;
  }
  return result;
}

}  // namespace rime
