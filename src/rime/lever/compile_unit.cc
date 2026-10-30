//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/build_config.h>

#include <algorithm>
#ifndef RIME_NO_THREADING
#include <condition_variable>
#include <fstream>
#include <mutex>
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

namespace {

constexpr uint64_t kEstimatedBytesPerSourceByte = 32;

#ifndef RIME_NO_THREADING
constexpr unsigned kMaxConcurrency = 4;
constexpr uint64_t kMemoryBudgetDivisor = 8;
constexpr uint64_t kFallbackMemoryBudget = 512ull * 1024 * 1024;
#endif  // !RIME_NO_THREADING

}  // namespace

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

#if defined(__linux__)
static uint64_t ReadMemoryLimitFile(const char* file) {
  std::ifstream fin{path(file)};
  string value;
  if (!(fin >> value) || value == "max") {
    return 0;
  }
  try {
    return std::stoull(value);
  } catch (...) {
    return 0;
  }
}
#endif

// Returns the smaller of physical memory and the cgroup memory limit, so that
// deployments inside a container respect the container's budget.
static uint64_t DetectMemoryLimit() {
  uint64_t limit = DetectTotalPhysicalMemory();
#if defined(__linux__)
  const char* cgroup_files[] = {
      "/sys/fs/cgroup/memory.max",
      "/sys/fs/cgroup/memory/memory.limit_in_bytes",
  };
  for (const auto& file : cgroup_files) {
    uint64_t cgroup_limit = ReadMemoryLimitFile(file);
    if (cgroup_limit > 0 && (limit == 0 || cgroup_limit < limit)) {
      limit = cgroup_limit;
    }
  }
#endif
  return limit;
}
#endif  // !RIME_NO_THREADING

CompileUnit::CompileUnit(string id,
                         uint64_t estimated_memory,
                         function<bool()> run)
    : id_(std::move(id)),
      estimated_memory_(estimated_memory),
      run_(std::move(run)) {}

void CompileUnit::AddOutput(string output) {
  outputs_.push_back(std::move(output));
}

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
                                             the<Config> schema_config,
                                             bool verbose,
                                             CompileUnit* unit) {
  Schema schema(schema_id, schema_config.release());
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
  path compiled_schema_path = compiled_resolver->ResolvePath(schema_id);
  an<Dictionary> dict_to_compile = std::move(dict);
  CompileUnit new_unit(schema_id, EstimateCompileMemory(dict_name),
                       [dict_to_compile, compiled_schema_path, verbose]() {
                         return CompileDictionary(dict_to_compile.get(),
                                                  compiled_schema_path,
                                                  verbose);
                       });
  // output files this unit may write; files shared by two units must not be
  // written concurrently
  for (const auto& table : dict_to_compile->tables()) {
    new_unit.AddOutput(table->file_path().stem().to_utf8_string());
  }
  new_unit.AddOutput(
      dict_to_compile->prism()->file_path().stem().to_utf8_string());
  new_unit.AddOutput(dict_to_compile->name());  // reverse db
  *unit = std::move(new_unit);
  return CompileUnitResult::kBuilt;
}

bool CompileGraph::AddUnit(CompileUnit unit) {
  if (unit.id().empty() || !unit.run()) {
    LOG(ERROR) << "invalid compile unit.";
    return false;
  }
  if (unit_index_.find(unit.id()) != unit_index_.end()) {
    LOG(WARNING) << "duplicate compile unit '" << unit.id() << "'.";
    return false;
  }
  unit_index_[unit.id()] = units_.size();
  units_.push_back(std::move(unit));
  return true;
}

bool CompileGraph::AddDependency(const string& prerequisite,
                                 const string& dependent) {
  auto prerequisite_it = unit_index_.find(prerequisite);
  auto dependent_it = unit_index_.find(dependent);
  if (prerequisite_it == unit_index_.end()) {
    LOG(WARNING) << "cannot order unknown compile unit '" << prerequisite
                 << "'.";
    return false;
  }
  if (dependent_it == unit_index_.end()) {
    LOG(WARNING) << "cannot order unknown compile unit '" << dependent << "'.";
    return false;
  }
  // Declaration order wins. This keeps output ordering acyclic and matches the
  // order in which schemas are listed.
  if (prerequisite_it->second >= dependent_it->second) {
    return true;
  }
  auto& prerequisites = units_[dependent_it->second].prerequisites_;
  if (std::find(prerequisites.begin(), prerequisites.end(),
                prerequisite_it->second) != prerequisites.end()) {
    return true;
  }
  prerequisites.push_back(prerequisite_it->second);
  return true;
}

bool CompileScheduler::RunUnit(const CompileUnit& unit) {
  LOG(INFO) << "compiling unit '" << unit.id() << "'.";
  if (!unit.run()) {
    LOG(ERROR) << "unit '" << unit.id() << "' has no work to run.";
    return false;
  }
  try {
    if (!unit.run()()) {
      LOG(ERROR) << "unit '" << unit.id() << "' failed to compile.";
      return false;
    }
  } catch (const std::bad_alloc& ex) {
    LOG(ERROR) << "unit '" << unit.id() << "' ran out of memory: " << ex.what();
    return false;
  } catch (const std::exception& ex) {
    LOG(ERROR) << "unit '" << unit.id()
               << "' threw an exception: " << ex.what();
    return false;
  } catch (...) {
    LOG(ERROR) << "unit '" << unit.id() << "' threw an unknown exception.";
    return false;
  }
  LOG(INFO) << "unit '" << unit.id() << "' is ready.";
  return true;
}

bool CompileScheduler::Run(const CompileGraph& graph, Result* result) {
  const size_t count = graph.size();
  if (count == 0) {
    return true;
  }

  vector<vector<size_t>> successors(count);
  vector<size_t> indegree(count, 0);
  auto add_edge = [&](size_t from, size_t to) {
    if (from == to) {
      return;
    }
    auto& next = successors[from];
    if (std::find(next.begin(), next.end(), to) != next.end()) {
      return;
    }
    next.push_back(to);
    ++indegree[to];
  };
  for (size_t i = 0; i < count; ++i) {
    for (size_t prerequisite : graph.unit(i).prerequisites()) {
      if (prerequisite >= count) {
        LOG(ERROR) << "compile unit '" << graph.unit(i).id()
                   << "' has an invalid prerequisite.";
        return false;
      }
      add_edge(prerequisite, i);
    }
  }
  // units writing the same output file are chained in declaration order
  map<string, size_t> last_writer;
  for (size_t i = 0; i < count; ++i) {
    for (const auto& output : graph.unit(i).outputs()) {
      auto inserted = last_writer.emplace(output, i);
      if (!inserted.second) {
        add_edge(inserted.first->second, i);
        inserted.first->second = i;
      }
    }
  }

  // Reject cycles before starting any work, so the scheduler can always make
  // progress.
  {
    vector<size_t> remaining_indegree = indegree;
    deque<size_t> ready;
    for (size_t i = 0; i < count; ++i) {
      if (remaining_indegree[i] == 0) {
        ready.push_back(i);
      }
    }
    size_t visited = 0;
    while (!ready.empty()) {
      size_t i = ready.front();
      ready.pop_front();
      ++visited;
      for (size_t successor : successors[i]) {
        if (--remaining_indegree[successor] == 0) {
          ready.push_back(successor);
        }
      }
    }
    if (visited != count) {
      LOG(ERROR) << "dependency cycle in compile graph.";
      return false;
    }
  }

#ifdef RIME_NO_THREADING
  deque<size_t> ready;
  for (size_t i = 0; i < count; ++i) {
    if (indegree[i] == 0) {
      ready.push_back(i);
    }
  }
  while (!ready.empty()) {
    size_t i = ready.front();
    ready.pop_front();
    if (RunUnit(graph.unit(i))) {
      ++result->success;
    } else {
      ++result->failure;
    }
    for (size_t successor : successors[i]) {
      if (--indegree[successor] == 0) {
        ready.push_back(successor);
      }
    }
  }
#else
  const unsigned concurrency =
      (std::min)((std::max)(1u, std::thread::hardware_concurrency()),
                 kMaxConcurrency);
  uint64_t memory_limit = DetectMemoryLimit();
  uint64_t memory_budget = memory_limit / kMemoryBudgetDivisor;
  if (memory_budget == 0) {
    memory_budget = memory_limit == 0 ? kFallbackMemoryBudget : memory_limit;
  }

  std::mutex mutex;
  std::condition_variable cv;
  deque<size_t> tasks;
  deque<pair<size_t, bool>> completed;
  bool stop = false;
  auto worker = [&]() {
    for (;;) {
      size_t index;
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return stop || !tasks.empty(); });
        if (tasks.empty()) {
          return;
        }
        index = tasks.front();
        tasks.pop_front();
      }
      bool ok = RunUnit(graph.unit(index));
      {
        std::lock_guard<std::mutex> lock(mutex);
        completed.emplace_back(index, ok);
      }
      cv.notify_all();
    }
  };
  vector<std::thread> workers;
  workers.reserve((std::min)((size_t)concurrency, count));
  for (size_t i = 0; i < (std::min)((size_t)concurrency, count); ++i) {
    workers.emplace_back(worker);
  }

  deque<size_t> ready;
  for (size_t i = 0; i < count; ++i) {
    if (indegree[i] == 0) {
      ready.push_back(i);
    }
  }
  size_t processed = 0;
  size_t in_flight = 0;
  uint64_t in_flight_memory = 0;
  auto finish_unit = [&](size_t index, bool ok) {
    if (ok) {
      ++result->success;
    } else {
      ++result->failure;
    }
    ++processed;
    for (size_t successor : successors[index]) {
      if (--indegree[successor] == 0) {
        ready.push_back(successor);
      }
    }
  };
  while (processed < count) {
    size_t inline_index = count;
    {
      std::lock_guard<std::mutex> lock(mutex);
      const size_t in_flight_before = in_flight;
      vector<size_t> dispatched;
      for (auto it = ready.begin();
           it != ready.end() && in_flight < concurrency;) {
        uint64_t estimated = graph.unit(*it).estimated_memory();
        bool fits =
            in_flight == 0 || in_flight_memory + estimated <= memory_budget;
        if (!fits) {
          ++it;
          continue;
        }
        dispatched.push_back(*it);
        in_flight_memory += estimated;
        ++in_flight;
        it = ready.erase(it);
      }
      if (in_flight_before == 0 && dispatched.size() == 1 && tasks.empty()) {
        in_flight_memory -= graph.unit(dispatched.front()).estimated_memory();
        --in_flight;
        inline_index = dispatched.front();
      } else {
        for (size_t index : dispatched) {
          tasks.push_back(index);
        }
      }
    }
    if (inline_index != count) {
      finish_unit(inline_index, RunUnit(graph.unit(inline_index)));
      continue;
    }
    cv.notify_all();

    std::pair<size_t, bool> completion;
    {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&] { return !completed.empty(); });
      completion = completed.front();
      completed.pop_front();
      in_flight_memory -= graph.unit(completion.first).estimated_memory();
      --in_flight;
    }
    finish_unit(completion.first, completion.second);
  }

  {
    std::lock_guard<std::mutex> lock(mutex);
    stop = true;
  }
  cv.notify_all();
  for (auto& worker_thread : workers) {
    worker_thread.join();
  }
#endif  // RIME_NO_THREADING
  return true;
}

}  // namespace rime
