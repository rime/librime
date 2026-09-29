//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/build_config.h>

#include <algorithm>
#ifndef RIME_NO_THREADING
#include <future>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif
#endif  // !RIME_NO_THREADING

#include <rime/common.h>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/ticket.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/lever/compile_unit.h>

namespace fs = std::filesystem;

namespace rime {

#ifndef RIME_NO_THREADING
static uint64_t DetectTotalPhysicalMemory() {
#ifdef _WIN32
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status)) {
    return status.ullTotalPhys;
  }
#elif defined(__APPLE__)
  int mib[2] = {CTL_HW, HW_MEMSIZE};
  uint64_t memsize = 0;
  size_t len = sizeof(memsize);
  if (sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0) {
    return memsize;
  }
#else
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    return (uint64_t)pages * (uint64_t)page_size;
  }
#endif
  return 0;
}
#endif  // !RIME_NO_THREADING

bool CompileDictionary(Dictionary* dict,
                       const path& compiled_schema_path,
                       bool verbose) {
  DictCompiler dict_compiler(dict);
  if (verbose) {
    dict_compiler.set_options(DictCompiler::kRebuild | DictCompiler::kDump);
  }
  return dict_compiler.Compile(compiled_schema_path);
}

uint64_t EstimateCompileMemory(const string& dict_name) {
  static const uint64_t kEstimatedBytesPerSourceByte = 32;
  the<ResourceResolver> source_resolver(
      Service::instance().CreateResourceResolver({"source_file", "", ""}));
  path dict_file = source_resolver->ResolvePath(dict_name + ".dict.yaml");
  uint64_t size = 0;
  std::error_code ec;
  if (auto s = fs::file_size(dict_file, ec); !ec) {
    size = (uint64_t)s;
  }
  return size * kEstimatedBytesPerSourceByte;
}

CompileUnitResult BuildDictionaryCompileUnit(const string& schema_id,
                                             Config* schema_config,
                                             bool verbose,
                                             CompileUnit* unit) {
  Schema schema(schema_id, schema_config);
  string dict_name;
  if (!schema.config()->GetString("translator/dictionary", &dict_name)) {
    return CompileUnitResult::kNoDictionary;
  }
  the<Dictionary> dict(
      Dictionary::Require("dictionary")->Create({&schema, "translator"}));
  if (!dict) {
    LOG(ERROR) << "Error creating dictionary '" << dict_name << "'.";
    return CompileUnitResult::kError;
  }
  the<ResourceResolver> compiled_resolver(
      Service::instance().CreateDeployedResourceResolver(
          {"compiled_schema", "", ".schema.yaml"}));
  CompileUnit new_unit;
  new_unit.id = schema_id;
  // output files this unit may write; files shared by two units must not be
  // written concurrently
  for (const auto& table : dict->tables()) {
    new_unit.outputs.push_back(table->file_path().stem().to_utf8_string());
  }
  new_unit.outputs.push_back(
      dict->prism()->file_path().stem().to_utf8_string());
  new_unit.outputs.push_back(dict->name());  // reverse db
  new_unit.estimated_memory = EstimateCompileMemory(dict_name);
  path compiled_schema_path = compiled_resolver->ResolvePath(schema_id);
  an<Dictionary> dict_to_compile = std::move(dict);
  new_unit.run = [dict_to_compile, compiled_schema_path, verbose]() {
    return CompileDictionary(dict_to_compile.get(), compiled_schema_path,
                             verbose);
  };
  *unit = std::move(new_unit);
  return CompileUnitResult::kBuilt;
}

bool CompileGraph::AddUnit(CompileUnit unit) {
  if (unit_index_.find(unit.id) != unit_index_.end()) {
    LOG(WARNING) << "duplicate compile unit '" << unit.id << "'.";
    return false;
  }
  unit_index_[unit.id] = units_.size();
  units_.push_back(std::move(unit));
  return true;
}

bool CompileGraph::AddDependency(const string& prerequisite,
                                 const string& dependent) {
  auto prerequisite_it = unit_index_.find(prerequisite);
  auto dependent_it = unit_index_.find(dependent);
  if (prerequisite_it == unit_index_.end() ||
      dependent_it == unit_index_.end()) {
    return false;
  }
  if (prerequisite_it->second >= dependent_it->second) {
    return false;
  }
  auto& dependencies = units_[dependent_it->second].dependencies;
  if (std::find(dependencies.begin(), dependencies.end(), prerequisite) !=
      dependencies.end()) {
    return true;
  }
  dependencies.push_back(prerequisite);
  return true;
}

bool CompileGraph::Build(vector<vector<size_t>>* levels) const {
  const size_t count = units_.size();
  levels->clear();
  vector<vector<size_t>> successors(count);
  vector<size_t> indegree(count, 0);
  auto add_edge = [&](size_t from, size_t to) {
    auto& next = successors[from];
    if (std::find(next.begin(), next.end(), to) != next.end()) {
      return;
    }
    next.push_back(to);
    ++indegree[to];
  };
  for (size_t i = 0; i < count; ++i) {
    for (const auto& dependency : units_[i].dependencies) {
      auto it = unit_index_.find(dependency);
      if (it != unit_index_.end()) {
        add_edge(it->second, i);
      }
    }
  }
  // units writing the same output file are chained in declaration order
  map<string, size_t> last_writer;
  for (size_t i = 0; i < count; ++i) {
    for (const auto& output : units_[i].outputs) {
      auto inserted = last_writer.emplace(output, i);
      if (!inserted.second) {
        add_edge(inserted.first->second, i);
        inserted.first->second = i;
      }
    }
  }
  // level(unit) = 1 + max(level(dependencies)); pick the smallest ready unit
  // so that levels are deterministic
  vector<size_t> level(count, 0);
  vector<bool> done(count, false);
  size_t remaining = count;
  while (remaining > 0) {
    size_t next = count;
    for (size_t i = 0; i < count; ++i) {
      if (!done[i] && indegree[i] == 0) {
        next = i;
        break;
      }
    }
    if (next == count) {
      LOG(ERROR) << "dependency cycle in compile graph.";
      return false;
    }
    done[next] = true;
    --remaining;
    if (levels->size() <= level[next]) {
      levels->resize(level[next] + 1);
    }
    (*levels)[level[next]].push_back(next);
    for (size_t successor : successors[next]) {
      level[successor] = (std::max)(level[successor], level[next] + 1);
      --indegree[successor];
    }
  }
  for (auto& level_units : *levels) {
    std::sort(level_units.begin(), level_units.end());
  }
  return true;
}

bool CompileScheduler::RunUnit(const CompileUnit& unit) {
  LOG(INFO) << "compiling unit '" << unit.id << "'.";
  try {
    if (!unit.run || !unit.run()) {
      LOG(ERROR) << "unit '" << unit.id << "' failed to compile.";
      return false;
    }
  } catch (const std::bad_alloc& ex) {
    LOG(ERROR) << "unit '" << unit.id << "' ran out of memory: " << ex.what();
    return false;
  } catch (const std::exception& ex) {
    LOG(ERROR) << "unit '" << unit.id << "' threw an exception: " << ex.what();
    return false;
  }
  LOG(INFO) << "unit '" << unit.id << "' is ready.";
  return true;
}

bool CompileScheduler::Run(const CompileGraph& graph, Result* result) {
  vector<vector<size_t>> levels;
  if (!graph.Build(&levels)) {
    return false;
  }
#ifdef RIME_NO_THREADING
  for (const auto& level_units : levels) {
    for (size_t i : level_units) {
      if (RunUnit(graph.unit(i))) {
        ++result->success;
      } else {
        ++result->failure;
      }
    }
  }
#else
  const unsigned kMaxConcurrency = 4;
  unsigned concurrency =
      (std::min)((std::max)(1u, std::thread::hardware_concurrency()),
                 kMaxConcurrency);
  // limit the memory used by one batch to a fraction of physical memory;
  // dictionary compilation is memory heavy (entry collector + trie/prism
  // building), so a conservative estimate per unit is used
  uint64_t memory_budget = DetectTotalPhysicalMemory() / 8;
  if (memory_budget == 0) {
    memory_budget = (uint64_t)512 * 1024 * 1024;  // fallback 512MB
  }
  for (const auto& level_units : levels) {
    if (level_units.empty()) {
      continue;
    }
    vector<size_t> batch_order = level_units;
    sort(batch_order.begin(), batch_order.end(), [&graph](size_t a, size_t b) {
      return graph.unit(a).estimated_memory > graph.unit(b).estimated_memory;
    });
    if (batch_order.size() > 1) {
      LOG(INFO) << "compiling " << batch_order.size() << " units in parallel.";
    }
    size_t begin = 0;
    while (begin < batch_order.size()) {
      size_t end = begin;
      uint64_t batch_memory = 0;
      unsigned batch_count = 0;
      while (end < batch_order.size() && batch_count < concurrency) {
        uint64_t estimated = graph.unit(batch_order[end]).estimated_memory;
        if (batch_memory + estimated > memory_budget) {
          break;
        }
        batch_memory += estimated;
        ++batch_count;
        ++end;
      }
      if (end == begin) {
        ++end;  // a single unit larger than the budget: compile alone
      }
      if (end - begin == 1) {
        if (RunUnit(graph.unit(batch_order[begin]))) {
          ++result->success;
        } else {
          ++result->failure;
        }
      } else {
        vector<std::future<bool>> futures;
        futures.reserve(end - begin);
        for (size_t k = begin; k < end; ++k) {
          size_t i = batch_order[k];
          futures.push_back(std::async(std::launch::async, [&graph, this, i]() {
            return RunUnit(graph.unit(i));
          }));
        }
        for (auto& future : futures) {
          try {
            if (future.get()) {
              ++result->success;
            } else {
              ++result->failure;
            }
          } catch (const std::exception& ex) {
            LOG(ERROR) << "parallel compilation failed: " << ex.what();
            ++result->failure;
          }
        }
      }
      begin = end;
    }
  }
#endif
  return true;
}

}  // namespace rime
