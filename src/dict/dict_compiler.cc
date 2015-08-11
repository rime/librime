//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/filesystem.hpp>
#include <rime/algo/algebra.h>
#include <rime/algo/utilities.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/entry_collector.h>
#include <rime/dict/preset_vocabulary.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/reverse_lookup_dictionary.h>

namespace rime {

DictCompiler::DictCompiler(Dictionary *dictionary, DictFileFinder finder)
    : dict_name_(dictionary->name()),
      prism_(dictionary->prism()),
      table_(dictionary->table()),
      dict_file_finder_(finder) {
}

bool DictCompiler::Compile(const string &schema_file) {
  LOG(INFO) << "compiling:";
  bool build_table_from_source = true;
  DictSettings settings;
  string dict_file(FindDictFile(dict_name_));
  if (dict_file.empty()) {
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
    string dict_file(FindDictFile(As<ConfigValue>(*it)->str()));
    if (dict_file.empty())
      return false;
    dict_files.push_back(dict_file);
  }
  uint32_t dict_file_checksum = 0;
  if (!dict_files.empty()) {
    ChecksumComputer cc;
    for (const auto& file_name : dict_files) {
      cc.ProcessFile(file_name);
    }
    if (settings.use_preset_vocabulary()) {
      cc.ProcessFile(PresetVocabulary::DictFilePath());
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
    ReverseDb reverse_db(dict_name_);
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

string DictCompiler::FindDictFile(const string& dict_name) {
  string dict_file(dict_name + ".dict.yaml");
  if (dict_file_finder_) {
    dict_file = dict_file_finder_(dict_file);
  }
  return dict_file;
}

bool DictCompiler::BuildTable(DictSettings* settings,
                              const vector<string>& dict_files,
                              uint32_t dict_file_checksum) {
  LOG(INFO) << "building table...";
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
      e->weight = r.weight;
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
  ReverseDb reverse_db(dict_name_);
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
                              uint32_t dict_file_checksum, uint32_t schema_file_checksum) {
  LOG(INFO) << "building prism...";
  // get syllabary from table
  Syllabary syllabary;
  if (!table_->Load() || !table_->GetSyllabary(&syllabary) || syllabary.empty())
    return false;
  // apply spelling algebra
  Script script;
  if (!schema_file.empty()) {
    Projection p;
    Config config(schema_file);
    auto algebra = config.GetList("speller/algebra");
    if (algebra && p.Load(algebra)) {
      for (const auto& x : syllabary) {
        script.AddSyllable(x);
      }
      if (!p.Apply(&script)) {
        script.clear();
      }
    }
  }
  if ((options_ & kDump) && !script.empty()) {
    boost::filesystem::path path(prism_->file_name());
    path.replace_extension(".txt");
    script.Dump(path.string());
  }
  // build .prism.bin
  {
    prism_->Remove();
    if (!prism_->Build(syllabary, script.empty() ? NULL : &script,
                       dict_file_checksum, schema_file_checksum) ||
        !prism_->Save()) {
      return false;
    }
  }
  return true;
}

}  // namespace rime
