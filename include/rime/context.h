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

class Composition;
class Segmentation; 

class Context {
 public:
  typedef boost::signal<void (Context *ctx)> Notifier;

  Context();
  ~Context();

  void Commit();
  const std::string GetCommitText() const;
  bool IsComposing() const;
  void PushInput(char ch);
  void PopInput();
  void Clear();

  void ConfirmCurrentSelection();
  
  void set_input(const std::string &value);
  const std::string& input() const { return input_; }

  void set_selector(int pos);
  const int selector() const { return selector_; }
  void set_cursor(int pos);
  const int cursor() const { return cursor_; }

  void set_composition(Composition *comp);
  Composition* composition();
  const Composition* composition() const;
  
  Notifier& commit_notifier() { return commit_notifier_; }
  Notifier& input_change_notifier() { return input_change_notifier_; }

 private:
  std::string input_;
  int selector_;
  int cursor_;
  scoped_ptr<Composition> composition_;
  
  Notifier commit_notifier_;
  Notifier input_change_notifier_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
