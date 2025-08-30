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

class StopSlotException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class RIME_DLL Context {
 public:
  using Notifier = signal<void(Context* ctx)>;
  using OptionUpdateNotifier = signal<void(Context* ctx, const string& option)>;
  using PropertyUpdateNotifier =
      signal<void(Context* ctx, const string& property)>;
  using KeyEventNotifier =
      signal<void(Context* ctx, const KeyEvent& key_event)>;

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
  // Clear and notify abort
  void AbortComposition();

  // return false if there is no candidate at index
  bool Select(size_t index);
  // return false if the selected index has not changed
  bool Highlight(size_t index);
  bool DeleteCandidate(size_t index);
  // return false if there's no candidate for current segment
  bool ConfirmCurrentSelection();
  bool DeleteCurrentSelection();
  void BeginEditing();
  bool ConfirmPreviousSelection();  // deprecated
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
  const map<string, bool>& options() const { return options_; }
  const map<string, string>& properties() const { return properties_; }
  // options and properties starting with '_' are local to schema;
  // others are session scoped.
  void ClearTransientOptions();
  template <typename T, typename... I>
  void notify(T& t, I&&... i) {
    try {
      t(i...);
    } catch (const StopSlotException& e) {
      LOG(INFO) << "stop slot " << e.what();
    }
  };
  void commit_notify() { notify(commit_notifier_, this); };
  void select_notify() {
    if (this->composition().empty())
      return;
    notify(select_notifier_, this);
  }
  void update_notify() { notify(update_notifier_, this); };
  void delete_notify() { notify(delete_notifier_, this); };
  void abort_notify() { notify(abort_notifier_, this); };
  void option_update_notify(const string& option) {
    notify(option_update_notifier_, this, option);
  };
  void property_update_notify(const string& property) {
    notify(property_update_notifier_, this, property);
  };
  void unhandled_key_notify(const rime::KeyEvent& key_event) {
    notify(unhandled_key_notifier_, this, key_event);
  };

  Notifier& commit_notifier() { return commit_notifier_; }
  Notifier& select_notifier() { return select_notifier_; }
  Notifier& update_notifier() { return update_notifier_; }
  Notifier& delete_notifier() { return delete_notifier_; }
  Notifier& abort_notifier() { return abort_notifier_; }
  OptionUpdateNotifier& option_update_notifier() {
    return option_update_notifier_;
  }
  PropertyUpdateNotifier& property_update_notifier() {
    return property_update_notifier_;
  }
  KeyEventNotifier& unhandled_key_notifier() { return unhandled_key_notifier_; }

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
  Notifier abort_notifier_;
  OptionUpdateNotifier option_update_notifier_;
  PropertyUpdateNotifier property_update_notifier_;
  KeyEventNotifier unhandled_key_notifier_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
