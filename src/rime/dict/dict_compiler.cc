//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <filesystem>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <boost/crc.hpp>
#include <rime/algo/algebra.h>
#include <rime/algo/utilities.h>
#include <rime/dict/corrector.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/entry_collector.h>
#include <rime/dict/preset_vocabulary.h>
#include <rime/dict/prism.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/dict/table.h>
#include <rime/resource.h>
#include <rime/service.h>

namespace rime {

DictCompiler::DictCompiler(Dictionary* dictionary)
    : dict_name_(dictionary->name()),
      packs_(dictionary->packs()),
      prism_(dictionary->prism()),
      tables_(dictionary->tables()),
      source_resolver_(
          Service::instance().CreateResourceResolver({"source_file", "", ""})),
      target_resolver_(Service::instance().CreateStagingResourceResolver(
          {"target_file", "", ""})) {}

DictCompiler::~DictCompiler() {}

static bool load_dict_settings_from_file(DictSettings* settings,
                                         const path& dict_file) {
  std::ifstream fin(dict_file.c_str());
  bool success = settings->LoadDictHeader(fin);
  fin.close();
  return success;
}

static bool get_dict_files_from_settings(vector<path>* dict_files,
                                         DictSettings& settings,
                                         ResourceResolver* source_resolver) {
  if (auto tables = settings.GetTables()) {
    for (auto it = tables->begin(); it != tables->end(); ++it) {
      string dict_name = As<ConfigValue>(*it)->str();
      auto dict_file = source_resolver->ResolvePath(dict_name + ".dict.yaml");
      if (!std::filesystem::exists(dict_file)) {
        LOG(ERROR) << "source file '" << dict_file << "' does not exist.";
        return false;
      }
      dict_files->push_back(dict_file);
    }
  }
  return true;
}

static uint32_t compute_dict_file_checksum(uint32_t initial_checksum,
                                           const vector<path>& dict_files,
                                           DictSettings& settings) {
  if (dict_files.empty()) {
    return initial_checksum;
  }
  ChecksumComputer cc(initial_checksum);
  for (const auto& file_path : dict_files) {
    cc.ProcessFile(file_path);
  }
  if (settings.use_preset_vocabulary()) {
    cc.ProcessFile(PresetVocabulary::DictFilePath(settings.vocabulary()));
  }
  return cc.Checksum();
}

bool DictCompiler::Compile(const path& schema_file) {
  LOG(INFO) << "compiling dictionary for " << schema_file;
  bool build_table_from_source = true;
  DictSettings settings;
  auto dict_file = source_resolver_->ResolvePath(dict_name_ + ".dict.yaml");
  if (!std::filesystem::exists(dict_file)) {
    LOG(ERROR) << "source file '" << dict_file << "' does not exist.";
    build_table_from_source = false;
  } else if (!load_dict_settings_from_file(&settings, dict_file)) {
    LOG(ERROR) << "failed to load settings from '" << dict_file << "'.";
    return false;
  }
  vector<path> dict_files;
  if (!get_dict_files_from_settings(&dict_files, settings,
                                    source_resolver_.get())) {
    return false;
  }
  uint32_t dict_file_checksum =
      compute_dict_file_checksum(0, dict_files, settings);
  uint32_t schema_file_checksum =
      schema_file.empty() ? 0 : Checksum(schema_file);
  bool rebuild_table = false;
  const auto& primary_table = tables_[0];
  if (primary_table->Exists() && primary_table->Load()) {
    if (build_table_from_source) {
      rebuild_table = primary_table->dict_file_checksum() != dict_file_checksum;
    } else {
      dict_file_checksum = primary_table->dict_file_checksum();
    }
    primary_table->Close();
  } else if (build_table_from_source) {
    rebuild_table = true;
  } else {
    LOG(ERROR) << "neither " << dict_name_ << ".dict.yaml nor " << dict_name_
               << ".table.bin exists.";
    return false;
  }
  LOG(INFO) << dict_file << "[" << dict_files.size() << " file(s)]"
            << " (" << dict_file_checksum << ")";
  LOG(INFO) << schema_file << " (" << schema_file_checksum << ")";
  {
    the<ResourceResolver> resolver(
        Service::instance().CreateDeployedResourceResolver(
            {"find_reverse_db", "", ".reverse.bin"}));
    ReverseDb reverse_db(resolver->ResolvePath(dict_name_));
    if (build_table_from_source &&
        (!reverse_db.Exists() || !reverse_db.Load() ||
         reverse_db.dict_file_checksum() != dict_file_checksum))
      rebuild_table = true;
  }
  if (build_table_from_source && (options_ & kRebuildTable)) {
    rebuild_table = true;
  }

  if (!rebuild_table)
    LOG(INFO) << "reuse existing table: " << primary_table->file_path();

  // Every table keeps its own syllable ids. Only the shared prism indexes
  // the union, so editing a pack never requires rebuilding the main table.
  Syllabary syllabary;
  if (rebuild_table) {
    EntryCollector collector;
    if (!BuildTable(0, collector, &settings, dict_files, dict_file_checksum)) {
      return false;
    }
    syllabary = std::move(collector.syllabary);
  } else if (!packs_.empty()) {
    if (!primary_table->Load())
      return false;
    const bool loaded = primary_table->GetSyllabary(&syllabary);
    primary_table->Close();
    if (!loaded)
      return false;
  }

  // Automatic encoding of pack phrases can reuse the primary dictionary's
  // readings without including its vocabulary in the pack.
  the<ReverseDb> primary_reverse;
  if (!packs_.empty()) {
    auto staged = target_resolver_->ResolvePath(dict_name_ + ".reverse.bin");
    primary_reverse.reset(new ReverseDb(staged));
    if (!primary_reverse->Load()) {
      the<ResourceResolver> resolver(
          Service::instance().CreateDeployedResourceResolver(
              {"reverse_db", "", ".reverse.bin"}));
      primary_reverse.reset(new ReverseDb(resolver->ResolvePath(dict_name_)));
      if (!primary_reverse->Load())
        primary_reverse.reset();
    }
  }
  auto lookup_primary = [&](const string& word) {
    string code;
    if (primary_reverse) {
      if (!primary_reverse->Lookup(word + "\x1fstem", &code))
        primary_reverse->Lookup(word, &code);
    }
    return code;
  };

  for (size_t i = 1; i < tables_.size(); ++i) {
    const string& pack_name = packs_[i - 1];
    auto& table = tables_[i];
    DictSettings pack_settings;
    auto pack_file = source_resolver_->ResolvePath(pack_name + ".dict.yaml");
    const bool has_source = std::filesystem::exists(pack_file);
    vector<path> pack_files;
    uint32_t pack_checksum = 0;
    if (has_source) {
      if (!load_dict_settings_from_file(&pack_settings, pack_file) ||
          !get_dict_files_from_settings(&pack_files, pack_settings,
                                        source_resolver_.get()))
        return false;
      // Keep pack caches independent of the primary dictionary checksum.
      // Bump the format marker to invalidate caches using the old seed.
      boost::crc_32_type format_crc;
      const char kIndependentPackFormat[] = "independent-pack-codes-v2";
      format_crc.process_bytes(kIndependentPackFormat,
                               sizeof(kIndependentPackFormat));
      const auto seed = format_crc.checksum();
      pack_checksum =
          compute_dict_file_checksum(seed, pack_files, pack_settings);
    }
    const bool pack_loaded = table->Exists() && table->Load();
    const bool rebuild_pack =
        has_source &&
        (!pack_loaded || table->dict_file_checksum() != pack_checksum ||
         (options_ & kRebuildTable));
    if (pack_loaded)
      table->Close();
    if (!has_source) {
      if (!pack_loaded) {
        LOG(WARNING) << "skipping unavailable pack '" << pack_name << "'.";
        continue;
      }
      LOG(INFO) << "pack source file '" << pack_file
                << "' does not exist, using prebuilt table '"
                << table->file_path() << "'";
    }
    if (rebuild_pack) {
      LOG(INFO) << "rebuilding pack '" << pack_name << "'";
      // Do not restrict the pack to the primary table's syllabary.
      EntryCollector collector;
      if (primary_reverse)
        collector.SetFallbackLookup(lookup_primary);
      if (!BuildTable(static_cast<int>(i), collector, &pack_settings,
                      pack_files, pack_checksum))
        return false;
      syllabary.insert(collector.syllabary.begin(), collector.syllabary.end());
    } else {
      LOG(INFO) << "pack '" << pack_name << "' reuses up-to-date table '"
                << table->file_path() << "'";
      if (!table->Load() || !table->GetSyllabary(&syllabary))
        return false;
      table->Close();
    }
  }

  // Preserve v5 and its original checksum when there are no packs.
  // With packs, weights alone do not invalidate the shared code index.
  uint32_t prism_checksum = dict_file_checksum;
  if (!packs_.empty()) {
    boost::crc_32_type checksum;
    for (const auto& code : syllabary)
      checksum.process_bytes(code.c_str(), code.size() + 1);
    prism_checksum = checksum.checksum();
  }
  bool rebuild_prism = true;
  if (prism_->Exists() && prism_->Load()) {
    rebuild_prism = prism_->dict_file_checksum() != prism_checksum ||
                    prism_->schema_file_checksum() != schema_file_checksum ||
                    (!packs_.empty() && !prism_->has_syllabary()) ||
                    (packs_.empty() && prism_->has_syllabary());
    prism_->Close();
  }
  if ((rebuild_prism || (options_ & kRebuildPrism)) &&
      !BuildPrism(schema_file, prism_checksum, schema_file_checksum, syllabary,
                  !packs_.empty()))
    return false;
  return true;
}

static path relocate_target(const path& source_path,
                            ResourceResolver* target_resolver) {
  auto resource_id = source_path.filename().to_utf8_string();
  return target_resolver->ResolvePath(resource_id);
}

bool DictCompiler::BuildTable(int table_index,
                              EntryCollector& collector,
                              DictSettings* settings,
                              const vector<path>& dict_files,
                              uint32_t dict_file_checksum) {
  auto& table = tables_[table_index];
  auto target_path =
      relocate_target(table->file_path(), target_resolver_.get());
  LOG(INFO) << "building table: " << target_path;
  table = New<Table>(target_path);

  collector.Configure(settings);
  collector.Collect(dict_files);
  if (options_ & kDump) {
    path dump_path(table->file_path());
    dump_path.replace_extension(".txt");
    collector.Dump(dump_path);
  }
  Vocabulary vocabulary;
  // build .table.bin
  {
    map<string, SyllableId> syllable_to_id;
    SyllableId syllable_id = 0;
    for (const auto& s : collector.syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    for (auto& r : collector.entries) {
      Code code;
      for (const auto& s : r.raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      // release memory in time to reduce memory usage
      RawCode().swap(r.raw_code);
      auto ls = vocabulary.LocateEntries(code);
      if (!ls) {
        LOG(ERROR) << "Error locating entries in vocabulary.";
        continue;
      }
      ShortDictEntry e;
      e.code.swap(code);
      e.text.swap(r.text);
      e.weight = log(r.weight > 0 ? r.weight : DBL_EPSILON);
      ls->push_back(std::move(e));
    }
    // release memory in time to reduce memory usage
    decltype(collector.entries)().swap(collector.entries);
    if (settings->sort_order() != "original") {
      vocabulary.SortHomophones();
    }
    table->Remove();
    if (!table->Build(collector.syllabary, vocabulary, collector.num_entries,
                      dict_file_checksum) ||
        !table->Save()) {
      return false;
    }
  }
  // build reverse db for the primary table
  if (table_index == 0 &&
      !BuildReverseDb(settings, collector.syllabary, vocabulary,
                      collector.stems, dict_file_checksum)) {
    return false;
  }
  return true;
}

bool DictCompiler::BuildReverseDb(DictSettings* settings,
                                  const Syllabary& syllabary,
                                  const Vocabulary& vocabulary,
                                  const ReverseLookupTable& stems,
                                  uint32_t dict_file_checksum) {
  // build .reverse.bin
  auto target_path = target_resolver_->ResolvePath(dict_name_ + ".reverse.bin");
  ReverseDb reverse_db(target_path);
  if (!reverse_db.Build(settings, syllabary, vocabulary, stems,
                        dict_file_checksum) ||
      !reverse_db.Save()) {
    LOG(ERROR) << "error building reversedb.";
    return false;
  }
  return true;
}

bool DictCompiler::BuildPrism(const path& schema_file,
                              uint32_t dict_file_checksum,
                              uint32_t schema_file_checksum,
                              const Syllabary& syllabary,
                              bool include_syllabary) {
  LOG(INFO) << "building prism...";
  auto target_path =
      relocate_target(prism_->file_path(), target_resolver_.get());
  prism_ = New<Prism>(target_path);

  // reuse the in-memory syllabary when available; otherwise load it from the
  // primary table, which may not have been rebuilt
  Syllabary loaded_syllabary;
  const Syllabary* syllables = &syllabary;
  if (syllables->empty()) {
    const auto& primary_table = tables_[0];
    if (!primary_table->Load() ||
        !primary_table->GetSyllabary(&loaded_syllabary) ||
        loaded_syllabary.empty())
      return false;
    syllables = &loaded_syllabary;
  }
  // apply spelling algebra and prepare corrections (if enabled)
  Script script;
  if (!schema_file.empty()) {
    Config config;
    if (!config.LoadFromFile(schema_file)) {
      LOG(ERROR) << "error loading prism definition from " << schema_file;
      return false;
    }
    Projection p;
    auto algebra = config.GetList("speller/algebra");
    if (algebra && p.Load(algebra)) {
      for (const auto& x : *syllables) {
        script.AddSyllable(x);
      }
      if (!p.Apply(&script)) {
        script.clear();
      }
    }

#if 0
    // build corrector
    bool enable_correction = false; // Avoid if initializer to comfort compilers
    if (config.GetBool("translator/enable_correction", &enable_correction) &&
        enable_correction) {
      path corrector_path(prism_->file_path());
      corrector_path.replace_extension("");
      corrector_path.replace_extension(".correction.bin");
      auto target_path = relocate_target(corrector_path,
                                         target_resolver_.get());
      correction_ = New<EditDistanceCorrector>(target_path);
      if (correction_->Exists()) {
        correction_->Remove();
      }
      if (!correction_->Build(*syllables, &script,
                         dict_file_checksum, schema_file_checksum) ||
          !correction_->Save()) {
        return false;
      }
    }
#endif
  }
  if ((options_ & kDump) && !script.empty()) {
    path dump_path(prism_->file_path());
    dump_path.replace_extension(".txt");
    script.Dump(dump_path);
  }
  // build .prism.bin
  {
    prism_->Remove();
    if (!prism_->Build(*syllables, script.empty() ? nullptr : &script,
                       dict_file_checksum, schema_file_checksum,
                       include_syllabary) ||
        !prism_->Save()) {
      return false;
    }
  }

  return true;
}

}  // namespace rime
