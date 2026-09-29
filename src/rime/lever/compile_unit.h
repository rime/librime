//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_COMPILE_UNIT_H_
#define RIME_COMPILE_UNIT_H_

#include <cstdint>

#include <rime/common.h>

namespace rime {

class Config;
class Dictionary;

// A unit of the compiling process. Each compiling module declares its
// dependencies and the files it writes; the deployer only executes the
// constructed graph of compile units.
struct CompileUnit {
  // unique id of the unit; for a schema's dictionary, the schema_id
  string id;
  // ids of units that must be compiled before this one
  vector<string> dependencies;
  // output file names (without extension) this unit may write; files shared
  // by two units must not be written concurrently
  vector<string> outputs;
  // estimated peak memory use in bytes, used for scheduling
  uint64_t estimated_memory = 0;
  // the actual work; returns false on failure
  function<bool()> run;
};

enum class CompileUnitResult {
  kBuilt,         // a compile unit was created
  kNoDictionary,  // the schema does not require a dictionary
  kError,         // failed to create the dictionary
};

// Creates a compile unit that compiles the dictionary of the given schema.
// Takes ownership of |schema_config| in every case.
CompileUnitResult BuildDictionaryCompileUnit(const string& schema_id,
                                             Config* schema_config,
                                             bool verbose,
                                             CompileUnit* unit);

// Estimates the peak memory of compiling the named dictionary from the size
// of its source file.
uint64_t EstimateCompileMemory(const string& dict_name);

// Compiles |dict| into artifacts resolved from |compiled_schema_path|.
bool CompileDictionary(Dictionary* dict,
                       const path& compiled_schema_path,
                       bool verbose = false);

// A graph of compile units: declared dependencies plus implicit ordering
// between units that write the same output file (declaration order first).
class CompileGraph {
 public:
  // returns false if the unit id is already used
  bool AddUnit(CompileUnit unit);
  // Declares that |prerequisite| is compiled before |dependent|; ignored when
  // either unit is unknown or when it contradicts declaration order (which
  // keeps the graph acyclic by construction).
  bool AddDependency(const string& prerequisite, const string& dependent);
  // Groups units into levels of pairwise independent units, such that all
  // dependencies of a unit are in earlier levels. Returns false and logs an
  // error if a dependency cycle is detected.
  bool Build(vector<vector<size_t>>* levels) const;

  size_t size() const { return units_.size(); }
  const CompileUnit& unit(size_t index) const { return units_[index]; }

 private:
  vector<CompileUnit> units_;
  map<string, size_t> unit_index_;
};

// Executes the compile units of a graph: in dependency order when threading
// is disabled, or in parallel batches bounded by hardware concurrency and a
// memory budget otherwise.
class CompileScheduler {
 public:
  struct Result {
    int success = 0;
    int failure = 0;
  };
  // Returns false if the graph is invalid; unit failures are counted.
  bool Run(const CompileGraph& graph, Result* result);

 private:
  bool RunUnit(const CompileUnit& unit);
};

}  // namespace rime

#endif  // RIME_COMPILE_UNIT_H_
