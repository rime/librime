//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <boost/filesystem.hpp>
#include <cfloat>
#include <cmath>
#include <fstream>
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

DictCompiler::DictCompiler(Dictionary *dictionary, const string& prefix)
    : dict_name_(dictionary->name()),
      prism_(dictionary->prism()),
      table_(dictionary->table()),
      prefix_(prefix) {
}

static string LocateFile(const string& file_name) {
  the<ResourceResolver> resolver(
      Service::instance().CreateResourceResolver({"build_source", "", ""}));
  return resolver->ResolvePath(file_name).string();
}

bool DictCompiler::Compile(const string &schema_file) {
  LOG(INFO) << "compiling dictionary for " << schema_file;
  bool build_table_from_source = true;
  DictSettings settings;
  string dict_file = LocateFile(dict_name_ + ".dict.yaml");
  if (!boost::filesystem::exists(dict_file)) {
    LOG(ERROR) << "source file '" << dict_file << "' does not exist.";
    build_table_from_source = false;
  }
  else {
    std::ifstream fin(dict_file.c_str());
    if (!settings.LoadDictHeader(fin)) {
      LOG(ERROR) << "failed to load settings from '" << dict_file << "'.";
      return false;
    }
    fin.close();
    LOG(INFO) << "dict name: " << settings.dict_name();
    LOG(INFO) << "dict version: " << settings.dict_version();
  }
  vector<string> dict_files;
  auto tables = settings.GetTables();
  for(auto it = tables->begin(); it != tables->end(); ++it) {
    if (!Is<ConfigValue>(*it))
      continue;
    string dict_name = As<ConfigValue>(*it)->str();
    string dict_file = LocateFile(dict_name + ".dict.yaml");
    if (!boost::filesystem::exists(dict_file)) {
      LOG(ERROR) << "source file '" << dict_file << "' does not exist.";
      return false;
    }
    dict_files.push_back(dict_file);
  }
  uint32_t dict_file_checksum = 0;
  if (!dict_files.empty()) {
    ChecksumComputer cc;
    for (const auto& file_name : dict_files) {
      cc.ProcessFile(file_name);
    }
    if (settings.use_preset_vocabulary()) {
      cc.ProcessFile(PresetVocabulary::DictFilePath(settings.vocabulary()));
    }
    dict_file_checksum = cc.Checksum();
  }
  uint32_t schema_file_checksum =
      schema_file.empty() ? 0 : Checksum(schema_file);
  bool rebuild_table = true;
  bool rebuild_prism = true;
  if (table_->Exists() && table_->Load()) {
    if (!build_table_from_source) {
      dict_file_checksum = table_->dict_file_checksum();
      LOG(INFO) << "reuse existing table: " << table_->file_name();
    }
    if (table_->dict_file_checksum() == dict_file_checksum) {
      rebuild_table = false;
    }
    table_->Close();
  }
  else if (!build_table_from_source) {
    LOG(ERROR) << "neither " << dict_name_ << ".dict.yaml nor "
        << dict_name_ << ".table.bin exists.";
    return false;
  }
  if (prism_->Exists() && prism_->Load()) {
    if (prism_->dict_file_checksum() == dict_file_checksum &&
        prism_->schema_file_checksum() == schema_file_checksum) {
      rebuild_prism = false;
    }
    prism_->Close();
  }
  LOG(INFO) << dict_file << "[" << dict_files.size() << " file(s)]"
            << " (" << dict_file_checksum << ")";
  LOG(INFO) << schema_file << " (" << schema_file_checksum << ")";
  {
    the<ResourceResolver> resolver(
        Service::instance().CreateResourceResolver(
            {"find_reverse_db", prefix_, ".reverse.bin"}));
    ReverseDb reverse_db(resolver->ResolvePath(dict_name_).string());
    if (!reverse_db.Exists() ||
        !reverse_db.Load() ||
        reverse_db.dict_file_checksum() != dict_file_checksum) {
      rebuild_table = true;
    }
  }
  if (build_table_from_source && (options_ & kRebuildTable)) {
    rebuild_table = true;
  }
  if (options_ & kRebuildPrism) {
    rebuild_prism = true;
  }
  if (rebuild_table && !BuildTable(&settings, dict_files, dict_file_checksum))
    return false;
  if (rebuild_prism && !BuildPrism(schema_file,
                                   dict_file_checksum, schema_file_checksum))
    return false;
  // done!
  return true;
}

static string RelocateToUserDirectory(const string& prefix,
                                      const string& file_name) {
  ResourceResolver resolver(ResourceType{"build_target", prefix, ""});
  resolver.set_root_path(Service::instance().deployer().user_data_dir);
  auto resource_id = boost::filesystem::path(file_name).filename().string();
  return resolver.ResolvePath(resource_id).string();
}

bool DictCompiler::BuildTable(DictSettings* settings,
                              const vector<string>& dict_files,
                              uint32_t dict_file_checksum) {
  LOG(INFO) << "building table...";
  table_ = New<Table>(RelocateToUserDirectory(prefix_, table_->file_name()));

  EntryCollector collector;
  collector.Configure(settings);
  collector.Collect(dict_files);
  if (options_ & kDump) {
    boost::filesystem::path path(table_->file_name());
    path.replace_extension(".txt");
    collector.Dump(path.string());
  }
  Vocabulary vocabulary;
  // build .table.bin
  {
    map<string, SyllableId> syllable_to_id;
    SyllableId syllable_id = 0;
    for (const auto& s : collector.syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    for (RawDictEntry& r : collector.entries) {
      Code code;
      for (const auto& s : r.raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      DictEntryList* ls = vocabulary.LocateEntries(code);
      if (!ls) {
        LOG(ERROR) << "Error locating entries in vocabulary.";
        continue;
      }
      auto e = New<DictEntry>();
      e->code.swap(code);
      e->text.swap(r.text);
      e->weight = log(r.weight > 0 ? r.weight : DBL_EPSILON);
      ls->push_back(e);
    }
    if (settings->sort_order() != "original") {
      vocabulary.SortHomophones();
    }
    table_->Remove();
    if (!table_->Build(collector.syllabary, vocabulary, collector.num_entries,
                       dict_file_checksum) ||
        !table_->Save()) {
      return false;
    }
  }
  // build .reverse.bin
  ReverseDb reverse_db(RelocateToUserDirectory(prefix_,
                                               dict_name_ + ".reverse.bin"));
  if (!reverse_db.Build(settings,
                        collector.syllabary,
                        vocabulary,
                        collector.stems,
                        dict_file_checksum)) {
    LOG(ERROR) << "error building reversedb.";
    return false;
  }
  return true;
}

bool DictCompiler::BuildPrism(const string &schema_file,
                              uint32_t dict_file_checksum,
                              uint32_t schema_file_checksum) {
  LOG(INFO) << "building prism...";
  prism_ = New<Prism>(RelocateToUserDirectory(prefix_, prism_->file_name()));

  // get syllabary from table
  Syllabary syllabary;
  if (!table_->Load() || !table_->GetSyllabary(&syllabary) || syllabary.empty())
    return false;
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
      for (const auto& x : syllabary) {
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
      boost::filesystem::path corrector_path(prism_->file_name());
      corrector_path.replace_extension("");
      corrector_path.replace_extension(".correction.bin");
      correction_ = New<EditDistanceCorrector>(RelocateToUserDirectory(prefix_, corrector_path.string()));
      if (correction_->Exists()) {
        correction_->Remove();
      }
      if (!correction_->Build(syllabary, &script,
                         dict_file_checksum, schema_file_checksum) ||
          !correction_->Save()) {
        return false;
      }
    }
#endif
  }
  if ((options_ & kDump) && !script.empty()) {
    boost::filesystem::path path(prism_->file_name());
    path.replace_extension(".txt");
    script.Dump(path.string());
  }
  // build .prism.bin
  {
    prism_->Remove();
    if (!prism_->Build(syllabary, script.empty() ? nullptr : &script,
                       dict_file_checksum, schema_file_checksum) ||
        !prism_->Save()) {
      return false;
    }
  }

  return true;
}

}  // namespace rime
