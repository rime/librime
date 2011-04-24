// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CONTEXT_H_
#define RIME_CONTEXT_H_

#include <string>
#include <boost/signals.hpp>
#include <rime/common.h>

namespace rime {

class Context {
 public:
  typedef boost::signal<void (Context *ctx,
                              const std::string &commit_text)> CommitNotifier;

  Context() {}
  ~Context() {}

  void Commit() {
    // TODO: echo...
    commit_notifier_(this, input());
  }
  bool IsComposing() const {
    return !input_.empty();
  }
  // TODO: notification on input change
  void PushInput(char ch) {
    input_.push_back(ch);
  }
  // TODO: notification on input change
  void PopInput() {
    if (!input_.empty())
      input_.resize(input_.size() - 1);
  }
  // TODO: notification on input change
  void Clear() {
    input_.clear();
  }
  // TODO: notification on input change
  void set_input(const std::string &value) {
    input_ = value;
  }
  const std::string& input() const {
    return input_;
  }
  CommitNotifier& commit_notifier() {
    return commit_notifier_;
  }

 private:
  std::string input_;
  CommitNotifier commit_notifier_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
