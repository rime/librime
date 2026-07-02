//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_OPENCC_H_
#define RIME_OPENCC_H_

#include <memory>
#include <rime/common.h>

namespace opencc {
class Converter;
class Dict;
}  // namespace opencc

namespace rime {

class Opencc {
 public:
  Opencc(const path& config_path);

  bool ConvertWord(const string& text, vector<string>* forms);
  bool RandomConvertText(const string& text, string* simplified);
  bool ConvertText(const string& text, string* simplified);

 private:
  void Initialize();

  bool initialized_;
  path config_path_;
  std::shared_ptr<opencc::Converter> converter_;
  std::shared_ptr<opencc::Dict> dict_;
};

}  // namespace rime

#endif  // RIME_OPENCC_H_
