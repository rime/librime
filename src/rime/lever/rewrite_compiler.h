//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_COMPILER_H_
#define RIME_REWRITE_COMPILER_H_

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class Config;
class Deployer;

class RIME_DLL RewriteCompiler {
 public:
  // Compiles native file stages and shared presets into reusable store stages.
  RewriteCompiler(const string& schema_id, Config* config, Deployer* deployer);

  bool Compile();
  static bool BeginWorkspace(Deployer* deployer);
  static bool AbortWorkspace(Deployer* deployer);
  static bool FinalizeWorkspace(const vector<string>& schema_ids,
                                Deployer* deployer);

 private:
  string schema_id_;
  Config* config_;
  Deployer* deployer_;
};

}  // namespace rime

#endif  // RIME_REWRITE_COMPILER_H_
