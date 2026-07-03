//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_OPENCC_H_
#define RIME_OPENCC_H_

#include <memory>
#include <rime_api.h>
#include <rime/common.h>

namespace opencc {
class Converter;
class Dict;
}  // namespace opencc

namespace rime {

class RIME_DLL Opencc {
 public:
  Opencc(const path& config_path);
  virtual ~Opencc() = default;

  virtual bool ConvertWord(const string& text, vector<string>* forms);
  virtual bool RandomConvertText(const string& text, string* simplified);
  virtual bool ConvertText(const string& text, string* simplified);

 private:
  void Initialize();

  bool initialized_;
  path config_path_;
  std::shared_ptr<opencc::Converter> converter_;
  std::shared_ptr<opencc::Dict> dict_;
};

}  // namespace rime

#endif  // RIME_OPENCC_H_
