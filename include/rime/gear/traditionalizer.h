#ifndef RIME_TRADITIONALIZER_H_
#define RIME_TRADITIONALIZER_H_

#include <rime/filter.h>

namespace rime {

class Opencc;

class Traditionalizer : public Filter {
 public:
  explicit Traditionalizer(Engine *engine);
  ~Traditionalizer();

  virtual bool Proceed(CandidateList *recruited,
                       CandidateList *candidates);

 protected:
  typedef enum { kTipNone, kTipChar, kTipAll } TipLevel;

  void Initialize();
  bool Convert(const shared_ptr<Candidate> &original,
               CandidateList *result);

  bool initialized_;
  scoped_ptr<Opencc> opencc_;
  TipLevel tip_level_;
  std::string option_name_;
  std::string opencc_config_;
};

}  // namespace rime

#endif  // RIME_TRADITIONALIZER_H_
