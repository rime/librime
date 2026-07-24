//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Benchmark: UserDictionary performance measurement.
// Build with -DRIME_USER_DICT_CACHE_ENABLED=0 to compare non-cache (DfsLookup)
// path.
//
#include <chrono>
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_dictionary.h>
#include <rime/dict/user_db.h>
#include <rime/dict/level_db.h>

using namespace rime;

using TestDb = UserDbWrapper<TextDb>;

static int fast_rand(int* seed) {
  int s = *seed;
  s = (s * 1103515245 + 12345) & 0x7fffffff;
  *seed = s;
  return s;
}

static string random_utf8_char(int* seed) {
  unsigned cp = 0x4e00 + (fast_rand(seed) % 0x3500);
  string s;
  if (cp < 0x800) {
    s += (char)(0xc0 | (cp >> 6));
    s += (char)(0x80 | (cp & 0x3f));
  } else {
    s += (char)(0xe0 | (cp >> 12));
    s += (char)(0x80 | ((cp >> 6) & 0x3f));
    s += (char)(0x80 | (cp & 0x3f));
  }
  return s;
}

// Pre-built system dictionary shared by all tests.
static an<Dictionary> sys_dict;
static vector<string> syllabary;

static void BuildSysDict() {
  if (sys_dict)
    return;
  sys_dict.reset(new Dictionary("dictionary_test", {},
                                {New<Table>(path{"dictionary_test.table.bin"})},
                                New<Prism>(path{"dictionary_test.prism.bin"})));
  sys_dict->Remove();
  DictCompiler dc(sys_dict.get());
  ASSERT_TRUE(dc.Compile(path()));
  ASSERT_TRUE(sys_dict->Load());

  Syllabary raw;
  ASSERT_TRUE(sys_dict->primary_table()->GetSyllabary(&raw));
  syllabary.assign(raw.begin(), raw.end());
  LOG(INFO) << "syllabary: " << syllabary.size() << " entries";
}

static string db_path(int id) {
  return (path{"user_dict_benchmark"} / std::to_string(id)).string() + ".txt";
}

static void populate_db(Db* db, size_t count) {
  int seed = 42;
  for (size_t i = 0; i < count;) {
    int n = 1 + (fast_rand(&seed) % 4);
    string code, text;
    for (int j = 0; j < n; ++j) {
      if (j > 0)
        code += ' ';
      code += syllabary[fast_rand(&seed) % syllabary.size()];
      text += random_utf8_char(&seed);
    }
    UserDbValue v;
    v.commits = 1 + (fast_rand(&seed) % 10);
    v.dee = (double)(fast_rand(&seed) % 10000) / 10000.0;
    v.tick = 500 + (fast_rand(&seed) % 500);
    if (db->Update(code + '\t' + text, v.Pack()))
      ++i;
  }
}

// ======================================================================
// Benchmark: Load (initialization)
// ======================================================================

TEST(UserDictBenchmarkLoad, DISABLED_BenchmarkLoad) {
  BuildSysDict();
  const size_t kEntryCounts[] = {10000, 50000, 100000};

  for (size_t n : kEntryCounts) {
    auto db = New<TestDb>(path{db_path(n)}, "benchmark");
    if (db->Exists())
      db->Remove();
    ASSERT_TRUE(db->Open());
    ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
    populate_db(db.get(), n);
    db->Close();

    // Benchmark: Open + Load
    auto db2 = New<TestDb>(path{db_path(n)}, "benchmark");
    ASSERT_TRUE(db2->Open());

    auto start = std::chrono::high_resolution_clock::now();
    {
      UserDictionary ud("benchmark", db2);
      ud.Attach(sys_dict->primary_table(), sys_dict->prism());
      ASSERT_TRUE(ud.Load());
    }
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - start)
                          .count();

    LOG(INFO) << "Load (" << n << " entries"
#if RIME_USER_DICT_CACHE_ENABLED
              << " +cache"
#else
              << " -cache"
#endif
              << "): " << (elapsed_us / 1000.0) << " ms";

    db2->Close();
    if (db2->Exists())
      db2->Remove();
  }
}

// ======================================================================
// Benchmark: Query (Lookup)
// ======================================================================

static double measure_lookup(UserDictionary* ud,
                             const string& input,
                             int iterations) {
  SyllableGraph g;
  Syllabifier syllabifier;
  int r = (syllabifier.BuildSyllableGraph(input, *sys_dict->prism(), &g));
  EXPECT_GT(r, 0);

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    auto result = ud->Lookup(g, 0);
  }
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
             .count() /
         (double)iterations;
}

class UserDictQueryBench : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { BuildSysDict(); }

  void SetUp() override {
    db = New<TestDb>(path{"bench_query.txt"}, "bench_query");
    if (db->Exists())
      db->Remove();
    ASSERT_TRUE(db->Open());
    ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
    populate_db(db.get(), 50000);
    ud = std::make_unique<UserDictionary>("bench_query", db);
    ud->Attach(sys_dict->primary_table(), sys_dict->prism());
    ASSERT_TRUE(ud->Load());
  }

  void TearDown() override {
    ud.reset();
    db->Close();
    if (db->Exists())
      db->Remove();
  }

  an<TestDb> db;
  the<UserDictionary> ud;
};

// ======================================================================
// LevelDB benchmarks (real-world performance comparison)
// ======================================================================

using Ldb = UserDbWrapper<LevelDb>;

class UserDictLdbQueryBench : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { BuildSysDict(); }

  void SetUp() override {
    home = path{"bench_ldb_query"};
    db = New<Ldb>(home, "bench_ldb_query");
    if (db->Exists())
      db->Remove();
    ASSERT_TRUE(db->Open());
    ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
    populate_db(db.get(), 50000);
    ud = std::make_unique<UserDictionary>("bench_ldb_query", db);
    ud->Attach(sys_dict->primary_table(), sys_dict->prism());
    ASSERT_TRUE(ud->Load());
  }

  void TearDown() override {
    ud.reset();
    db->Close();
    if (db->Exists())
      db->Remove();
  }

  an<Ldb> db;
  the<UserDictionary> ud;
  path home;
};

TEST_F(UserDictLdbQueryBench, DISABLED_BenchmarkLdbLookupNhm) {
  double us = measure_lookup(ud.get(), "nhm", 50);
  LOG(INFO) << "Lookup (nhm) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

TEST_F(UserDictLdbQueryBench, DISABLED_BenchmarkLdbLookupFiveLetter) {
  double us = measure_lookup(ud.get(), "nhmsh", 50);
  LOG(INFO) << "Lookup (nhmsh) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

TEST_F(UserDictLdbQueryBench, DISABLED_BenchmarkLdbLookupSevenLetter) {
  double us = measure_lookup(ud.get(), "nhmshsh", 50);
  LOG(INFO) << "Lookup (nhmshsh) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

TEST(UserDictBenchmarkSaveLdb, DISABLED_BenchmarkLdbUpdateEntry) {
  BuildSysDict();
  auto db = New<Ldb>(path{"bench_ldb_save"}, "bench_ldb_save");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));

  UserDictionary ud("bench_ldb_save", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kEntries = 5000;
  int seed = 789;

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kEntries; ++i) {
    int n = 1 + (fast_rand(&seed) % 4);
    string code, text;
    for (int j = 0; j < n; ++j) {
      if (j > 0)
        code += ' ';
      code += syllabary[fast_rand(&seed) % syllabary.size()];
      text += random_utf8_char(&seed);
    }
    DictEntry e;
    e.text = text;
    e.code.resize(n, 0);
    e.custom_code = code;
    e.commit_count = 1;
    e.weight = 0.5;
    ud.UpdateEntry(e, 1);
  }
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count();

  LOG(INFO) << "UpdateEntry (" << kEntries << " entries) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed / kEntries) << " us per entry, "
            << (elapsed / 1000) << " ms total";

  db->Close();
  if (db->Exists())
    db->Remove();
}

TEST(UserDictBenchmarkTxnLdb, DISABLED_BenchmarkLdbTransaction) {
  BuildSysDict();
  auto db = New<Ldb>(path{"bench_ldb_txn"}, "bench_ldb_txn");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));

  UserDictionary ud("bench_ldb_txn", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kEntries = 2000;
  int seed = 111;
  bool has_txn = ud.NewTransaction();

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kEntries; ++i) {
    int n = 1 + (fast_rand(&seed) % 4);
    string code, text;
    for (int j = 0; j < n; ++j) {
      if (j > 0)
        code += ' ';
      code += syllabary[fast_rand(&seed) % syllabary.size()];
      text += random_utf8_char(&seed);
    }
    DictEntry e;
    e.text = text;
    e.code.resize(n, 0);
    e.custom_code = code;
    e.commit_count = 1;
    e.weight = 0.5;
    ud.UpdateEntry(e, 1);
  }
  if (has_txn) {
    ud.CommitPendingTransaction();
  }
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count();

  LOG(INFO) << "UpdateBatch (" << kEntries << " entries) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed / kEntries) << " us per entry, "
            << (elapsed / 1000) << " ms total"
            << (has_txn ? " (with txn)" : " (no txn)");

  db->Close();
  if (db->Exists())
    db->Remove();
}

TEST(UserDictBenchmarkLoadLdb, DISABLED_BenchmarkLdbLoad) {
  BuildSysDict();
  const size_t kEntryCounts[] = {10000, 50000, 100000};

  for (size_t n : kEntryCounts) {
    auto name = "bench_ldb_load_" + std::to_string(n);
    auto db = New<Ldb>(path{name}, name);
    if (db->Exists())
      db->Remove();
    ASSERT_TRUE(db->Open());
    ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
    populate_db(db.get(), n);
    db->Close();

    auto db2 = New<Ldb>(path{name}, name);
    ASSERT_TRUE(db2->Open());

    auto start = std::chrono::high_resolution_clock::now();
    {
      UserDictionary ud("bench_ldb_load", db2);
      ud.Attach(sys_dict->primary_table(), sys_dict->prism());
      ASSERT_TRUE(ud.Load());
    }
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - start)
                          .count();

    LOG(INFO) << "Load (" << n << " entries) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
              << " +cache"
#else
              << " -cache"
#endif
              << ": " << (elapsed_us / 1000.0) << " ms";

    db2->Close();
    if (db2->Exists())
      db2->Remove();
  }
}

TEST(UserDictBenchmarkReloadLdb, DISABLED_BenchmarkLdbReload) {
  BuildSysDict();
  auto db = New<Ldb>(path{"bench_ldb_reload"}, "bench_ldb_reload");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
  populate_db(db.get(), 50000);

  UserDictionary ud("bench_ldb_reload", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kIterations = 10;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    ud.Reload();
  }
  auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - start)
                        .count();

  LOG(INFO) << "Reload (50000 entries) [LevelDB]"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed_us / 1000.0 / kIterations) << " ms per call";

  db->Close();
  if (db->Exists())
    db->Remove();
}

// ======================================================================
// Benchmark: Query (Lookup) — TextDb
// ======================================================================

TEST_F(UserDictQueryBench, DISABLED_BenchmarkLookupNhm) {
  double us = measure_lookup(ud.get(), "nhm", 50);
  LOG(INFO) << "Lookup (nhm)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

TEST_F(UserDictQueryBench, DISABLED_BenchmarkLookupFiveLetter) {
  double us = measure_lookup(ud.get(), "nhmsh", 50);
  LOG(INFO) << "Lookup (nhmsh)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

TEST_F(UserDictQueryBench, DISABLED_BenchmarkLookupSevenLetter) {
  double us = measure_lookup(ud.get(), "nhmshsh", 50);
  LOG(INFO) << "Lookup (nhmshsh)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << us << " us";
}

// ======================================================================
// Benchmark: Update (save)
// ======================================================================

TEST(UserDictBenchmarkSave, DISABLED_BenchmarkUpdateEntry) {
  BuildSysDict();
  auto db = New<TestDb>(path{"bench_save.txt"}, "bench_save");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));

  UserDictionary ud("bench_save", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kEntries = 5000;
  int seed = 123;
  TickCount t = 1000;

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kEntries; ++i) {
    int n = 1 + (fast_rand(&seed) % 4);
    string code, text;
    for (int j = 0; j < n; ++j) {
      if (j > 0)
        code += ' ';
      code += syllabary[fast_rand(&seed) % syllabary.size()];
      text += random_utf8_char(&seed);
    }
    DictEntry e;
    e.text = text;
    e.code.resize(n, 0);
    e.custom_code = code;
    e.commit_count = 1;
    e.weight = 0.5;
    ud.UpdateEntry(e, 1);
    ++t;
  }
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count();

  LOG(INFO) << "UpdateEntry (" << kEntries << " entries)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed / kEntries) << " us per entry, "
            << (elapsed / 1000) << " ms total";

  db->Close();
  if (db->Exists())
    db->Remove();
}

// ======================================================================
// Benchmark: Transaction (sync — batch write + commit)
// ======================================================================

TEST(UserDictBenchmarkTransaction, DISABLED_BenchmarkTransaction) {
  BuildSysDict();
  auto db = New<TestDb>(path{"bench_txn.txt"}, "bench_txn");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));

  UserDictionary ud("bench_txn", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kEntries = 2000;
  int seed = 456;

  bool has_txn = ud.NewTransaction();

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kEntries; ++i) {
    int n = 1 + (fast_rand(&seed) % 4);
    string code, text;
    for (int j = 0; j < n; ++j) {
      if (j > 0)
        code += ' ';
      code += syllabary[fast_rand(&seed) % syllabary.size()];
      text += random_utf8_char(&seed);
    }
    DictEntry e;
    e.text = text;
    e.code.resize(n, 0);
    e.custom_code = code;
    e.commit_count = 1;
    e.weight = 0.5;
    ud.UpdateEntry(e, 1);
  }
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count();

  LOG(INFO) << "UpdateBatch (" << kEntries << " entries)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed / kEntries) << " us per entry, "
            << (elapsed / 1000) << " ms total"
            << (has_txn ? " (with txn)" : " (no txn support)");

  if (has_txn) {
    ud.CommitPendingTransaction();
  }

  db->Close();
  if (db->Exists())
    db->Remove();
}

// ======================================================================
// Benchmark: Reload (sync — rebuild cache after external change)
// ======================================================================

TEST(UserDictBenchmarkReload, DISABLED_BenchmarkReload) {
  BuildSysDict();
  auto db = New<TestDb>(path{"bench_reload.txt"}, "bench_reload");
  if (db->Exists())
    db->Remove();
  ASSERT_TRUE(db->Open());
  ASSERT_TRUE(db->MetaUpdate("/tick", "1000"));
  populate_db(db.get(), 50000);

  UserDictionary ud("bench_reload", db);
  ud.Attach(sys_dict->primary_table(), sys_dict->prism());
  ASSERT_TRUE(ud.Load());

  const int kIterations = 10;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    ud.Reload();
  }
  auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - start)
                        .count();

  LOG(INFO) << "Reload (50000 entries)"
#if RIME_USER_DICT_CACHE_ENABLED
            << " +cache"
#else
            << " -cache"
#endif
            << ": " << (elapsed_us / 1000.0 / kIterations) << " ms per call";

  db->Close();
  if (db->Exists())
    db->Remove();
}
