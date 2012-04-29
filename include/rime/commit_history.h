// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMIT_HISTORY_H_
#define RIME_COMMIT_HISTORY_H_

#include <list>
#include <string>

namespace rime {

struct CommitRecord {
  std::string type;
  std::string text;
  CommitRecord(const std::string& a_type, const std::string& a_text)
      : type(a_type), text(a_text) {}
  CommitRecord(int keycode) : type("thru"), text(1, keycode) {}
};

class KeyEvent;
class Composition;

class CommitHistory : public std::list<CommitRecord> {
 public:
  static const size_t kMaxRecords = 20;
  void Push(const CommitRecord& record);
  void Push(const KeyEvent& key_event);
  void Push(const Composition& composition, const std::string& input);
  const std::string repr() const;
};

}  // Namespace rime

#endif  // RIME_COMMIT_HISTORY_H_
