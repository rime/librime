#ifndef RIME_OPENCC_H_
#define RIME_OPENCC_H_

#include <string>
#include <opencc/opencc.h>

namespace rime {

class Opencc {
public:
  Opencc(const std::string &config_path);
  ~Opencc();
  bool ConvertText(const std::string &text, std::string *simplified, bool *is_single_char);

private:
  opencc_t od_;
};

}  // namespace rime

#endif  // RIME_OPENCC_H_
