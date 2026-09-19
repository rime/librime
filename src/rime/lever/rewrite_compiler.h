//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_COMPILER_H_
#define RIME_REWRITE_COMPILER_H_

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class Deployer;
class Schema;

class RIME_DLL RewriteCompiler {
 public:
  // Compiles only the native section/files contract. External data importers
  // are intentionally outside this class.
  RewriteCompiler(Schema* schema, Deployer* deployer);

  bool Compile();

 private:
  Schema* schema_;
  Deployer* deployer_;
};

}  // namespace rime

#endif  // RIME_REWRITE_COMPILER_H_
