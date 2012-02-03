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
struct Preedit;

class Context {
 public:
  typedef boost::signal<void (Context *ctx)> Notifier;
  typedef boost::signal<void (Context *ctx, const std::string& option)>
  OptionUpdateNotifier;

  Context();
  ~Context();

  bool Commit();
  const std::string GetCommitText() const;
  const std::string GetScriptText() const;
  void GetPreedit(Preedit *preedit) const;
  bool IsComposing() const;
  bool HasMenu() const;
  
  bool PushInput(char ch);
  bool PopInput();
  bool DeleteInput();
  void Clear();

  // return false if there is no candidate at index
  bool Select(size_t index);
  // return false if there's no candidate for current segment
  bool ConfirmCurrentSelection();
  
  bool ConfirmPreviousSelection();
  bool ReopenPreviousSegment();
  bool ReopenPreviousSelection();
  bool ClearNonConfirmedComposition();
  bool RefreshNonConfirmedComposition();

  void set_prompt(const std::string &value) { prompt_ = value; }
  void clear_prompt() { prompt_.clear(); }
  const std::string& prompt() const { return prompt_; }

  void set_input(const std::string &value);
  const std::string& input() const { return input_; }

  void set_caret_pos(size_t caret_pos);
  const size_t caret_pos() const { return caret_pos_; }

  void set_composition(Composition *comp);
  Composition* composition();
  const Composition* composition() const;

  void set_option(const std::string &name, bool value);
  bool get_option(const std::string &name) const;
  
  Notifier& commit_notifier() { return commit_notifier_; }
  Notifier& select_notifier() { return select_notifier_; }
  Notifier& update_notifier() { return update_notifier_; }
  OptionUpdateNotifier& option_update_notifier() {
    return option_update_notifier_;
  }

 private:
  std::string prompt_;
  std::string input_;
  size_t caret_pos_;
  scoped_ptr<Composition> composition_;
  std::map<std::string, bool> options_;

  Notifier commit_notifier_;
  Notifier select_notifier_;
  Notifier update_notifier_;
  OptionUpdateNotifier option_update_notifier_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
