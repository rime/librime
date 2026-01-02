// src/rime/gear/sequence_adjuster.h
#ifndef RIME_SEQUENCE_ADJUSTER_H_
#define RIME_SEQUENCE_ADJUSTER_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>
#include <rime/filter.h>
#include <rime/dict/db.h>
#include <rime/key_event.h>

namespace rime {

class Context;

// 1. 数据结构定义
struct SequenceDbValue {
  int position = -1;       // p: 目标位置
  unsigned long stamp = 0; // s: 物理时间戳 (stamp)

  std::string Pack() const;
  static SequenceDbValue Unpack(const std::string& value);
};

// 2. Processor 定义
class SequenceAdjusterProcessor : public Processor {
 public:
  SequenceAdjusterProcessor(const Ticket& ticket);
  ~SequenceAdjusterProcessor();

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

 protected:
  void LoadConfig();
  bool SaveAdjustment(Context* ctx, int offset, bool is_pin, bool is_reset);
  static std::string MakeDbKey(const std::string& code, const std::string& text);

  an<Db> user_db_;
  std::string db_name_; 
  
  KeyEvent key_up_;
  KeyEvent key_down_;
  KeyEvent key_pin_;
  KeyEvent key_reset_;
};

// 3. Filter 定义
class SequenceAdjusterFilter : public Filter {
 public:
  SequenceAdjusterFilter(const Ticket& ticket);
  ~SequenceAdjusterFilter();

  an<Translation> Apply(an<Translation> translation,
                        CandidateList* candidates) override;

 protected:
  static std::string MakeDbKey(const std::string& code, const std::string& text);

  an<Db> user_db_;
  std::string db_name_; 
};

}  // namespace rime

#endif  // RIME_SEQUENCE_ADJUSTER_H_