// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-22 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CANDIDATE_H_
#define RIME_CANDIDATE_H_

#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

class Candidate {
 public:
  Candidate();
  Candidate(const std::string type,
            const std::string text,
            const std::string prompt,
            int start,
            int end,
            int order);
  virtual ~Candidate();

  const std::string& type() const { return type_; }
  const std::string& text() const { return text_; }
  const std::string& prompt() const { return prompt_; }
  int start() const { return start_; }
  int end() const { return end_; }
  int order() const { return order_; }

  void set_type(const std::string &type) { type_ = type; }
  void set_text(const std::string &text) { text_ = text; }
  void set_prompt(const std::string &prompt) { prompt_ = prompt; }
  void set_start(int start) { start_ = start; }
  void set_end(int end) { end_ = end; }
  void set_order(int order) { order_ = order; }

 private:
  std::string type_;
  std::string text_;
  std::string prompt_;
  int start_;
  int end_;
  int order_;
};

typedef std::vector<shared_ptr<Candidate> > CandidateList;

}  // namespace rime

#endif  // RIME_CANDIDATE_H_
