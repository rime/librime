//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CONTEXT_H_
#define RIME_CONTEXT_H_

#include <rime/common.h>
#include <rime/commit_history.h>
#include <rime/composition.h>

namespace rime {

class Candidate;
class KeyEvent;

class Context {
 public:
  using Notifier = signal<void (Context* ctx)>;
  using OptionUpdateNotifier =
      signal<void (Context* ctx, const string& option)>;
  using PropertyUpdateNotifier =
      signal<void (Context* ctx, const string& property)>;
  using KeyEventNotifier =
      signal<void (Context* ctx, const KeyEvent& key_event)>;

  Context() = default;
  ~Context() = default;

  bool Commit();
  string GetCommitText() const;
  string GetScriptText() const;
  Preedit GetPreedit() const;
  bool IsComposing() const;
  bool HasMenu() const;
  an<Candidate> GetSelectedCandidate() const;

  bool PushInput(char ch);
  bool PushInput(const string& str);
  bool PopInput(size_t len = 1);
  bool DeleteInput(size_t len = 1);
  void Clear();

  // return false if there is no candidate at index
  bool Select(size_t index);
  // return false if there's no candidate for current segment
  bool ConfirmCurrentSelection();
  bool DeleteCurrentSelection();

  bool ConfirmPreviousSelection();
  bool ReopenPreviousSegment();
  bool ClearPreviousSegment();
  bool ReopenPreviousSelection();
  bool ClearNonConfirmedComposition();
  bool RefreshNonConfirmedComposition();

  void set_input(const string& value);
  const string& input() const { return input_; }

  void set_caret_pos(size_t caret_pos);
  size_t caret_pos() const { return caret_pos_; }

  void set_composition(Composition&& comp);
  Composition& composition() { return composition_; }
  const Composition& composition() const { return composition_; }
  CommitHistory& commit_history() { return commit_history_; }
  const CommitHistory& commit_history() const { return commit_history_; }

  void set_option(const string& name, bool value);
  bool get_option(const string& name) const;
  void set_property(const string& name, const string& value);
  string get_property(const string& name) const;
  // options and properties starting with '_' are local to schema;
  // others are session scoped.
  void ClearTransientOptions();

  Notifier& commit_notifier() { return commit_notifier_; }
  Notifier& select_notifier() { return select_notifier_; }
  Notifier& update_notifier() { return update_notifier_; }
  Notifier& delete_notifier() { return delete_notifier_; }
  OptionUpdateNotifier& option_update_notifier() {
    return option_update_notifier_;
  }
  PropertyUpdateNotifier& property_update_notifier() {
    return property_update_notifier_;
  }
  KeyEventNotifier& unhandled_key_notifier() {
    return unhandled_key_notifier_;
  }

 private:
  string GetSoftCursor() const;

  string input_;
  size_t caret_pos_ = 0;
  Composition composition_;
  CommitHistory commit_history_;
  map<string, bool> options_;
  map<string, string> properties_;

  Notifier commit_notifier_;
  Notifier select_notifier_;
  Notifier update_notifier_;
  Notifier delete_notifier_;
  OptionUpdateNotifier option_update_notifier_;
  PropertyUpdateNotifier property_update_notifier_;
  KeyEventNotifier unhandled_key_notifier_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
