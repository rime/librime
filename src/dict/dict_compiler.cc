//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <map>
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/algo/algebra.h>
#include <rime/algo/utilities.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/entry_collector.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/reverse_lookup_dictionary.h>

namespace rime {

DictCompiler::DictCompiler(Dictionary *dictionary, DictFileFinder finder)
    : dict_name_(dictionary->name()),
      prism_(dictionary->prism()),
      table_(dictionary->table()),
      options_(0),
      dict_file_finder_(finder) {
}

bool DictCompiler::Compile(const std::string &schema_file) {
  LOG(INFO) << "compiling:";
  std::string dict_file(FindDictFile(dict_name_));
  if (dict_file.empty())
    return false;
  std::ifstream fin(dict_file.c_str());
  DictSettings settings;
  if (!settings.LoadDictHeader(fin)) {
    LOG(ERROR) << "failed to load settings from '" << dict_file << "'.";
    return false;
  }
  fin.close();
  LOG(INFO) << "dict name: " << settings.dict_name();
  LOG(INFO) << "dict version: " << settings.dict_version();
  std::vector<std::string> dict_files;
  ConfigListPtr tables = settings.GetTables();
  for(ConfigList::Iterator it = tables->begin(); it != tables->end(); ++it) {
    if (!Is<ConfigValue>(*it))
      continue;
    std::string dict_file(FindDictFile(As<ConfigValue>(*it)->str()));
    if (dict_file.empty())
      return false;
    dict_files.push_back(dict_file);
  }
  uint32_t dict_file_checksum =
      dict_file.empty() ? 0 : Checksum(dict_files);
  uint32_t schema_file_checksum =
      schema_file.empty() ? 0 : Checksum(schema_file);
  LOG(INFO) << dict_file << "[" << dict_files.size() << "]"
            << " (" << dict_file_checksum << ")";
  LOG(INFO) << schema_file << " (" << schema_file_checksum << ")";
  bool rebuild_table = true;
  bool rebuild_prism = true;
  bool rebuild_rev_lookup_dict = true;
  if (boost::filesystem::exists(table_->file_name()) && table_->Load()) {
    if (table_->dict_file_checksum() == dict_file_checksum) {
      rebuild_table = false;
    }
    table_->Close();
  }
  if (boost::filesystem::exists(prism_->file_name()) && prism_->Load()) {
    if (prism_->dict_file_checksum() == dict_file_checksum &&
        prism_->schema_file_checksum() == schema_file_checksum) {
      rebuild_prism = false;
    }
    prism_->Close();
  }
  TreeDb deprecated_db(dict_name_ + ".reverse.kct", "reversedb");
  if (deprecated_db.Exists()) {
    deprecated_db.Remove();
    LOG(INFO) << "removed deprecated db '" << deprecated_db.name() << "'.";
  }
  ReverseDb db(dict_name_);
  if (db.Exists() && db.OpenReadOnly()) {
    std::string checksum;
    if (db.MetaFetch("/dict_file_checksum", &checksum) &&
        boost::lexical_cast<uint32_t>(checksum) == dict_file_checksum) {
      rebuild_rev_lookup_dict = false;
    }
    db.Close();
  }
  if (options_ & kRebuildTable) {
    rebuild_table = true;
    rebuild_rev_lookup_dict = true;
  }
  if (options_ & kRebuildPrism) {
    rebuild_prism = true;
  }
  if (rebuild_table && !BuildTable(&settings, dict_files, dict_file_checksum))
    return false;
  if (rebuild_prism && !BuildPrism(schema_file,
                                   dict_file_checksum, schema_file_checksum))
    return false;
  if (rebuild_rev_lookup_dict &&
      !BuildReverseLookupDict(&db, dict_file_checksum))
    return false;
  // done!
  return true;
}

std::string DictCompiler::FindDictFile(const std::string& dict_name) {
  std::string dict_file(dict_name + ".dict.yaml");
  if (dict_file_finder_) {
    dict_file = dict_file_finder_(dict_file);
  }
  return dict_file;
}

bool DictCompiler::BuildTable(DictSettings* settings,
                              const std::vector<std::string>& dict_files,
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
  // build table
  {
    std::map<std::string, int> syllable_to_id;
    int syllable_id = 0;
    BOOST_FOREACH(const std::string &s, collector.syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    Vocabulary vocabulary;
    BOOST_FOREACH(RawDictEntry &r, collector.entries) {
      Code code;
      BOOST_FOREACH(const std::string &s, r.raw_code) {
        code.push_back(syllable_to_id[s]);
      }
      DictEntryList *ls = vocabulary.LocateEntries(code);
      if (!ls) {
        LOG(ERROR) << "Error locating entries in vocabulary.";
        continue;
      }
      shared_ptr<DictEntry> e = make_shared<DictEntry>();
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
  return true;
}

bool DictCompiler::BuildPrism(const std::string &schema_file,
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
    ConfigListPtr algebra = config.GetList("speller/algebra");
    if (algebra && p.Load(algebra)) {
      BOOST_FOREACH(Syllabary::value_type const& x, syllabary) {
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
  // build prism
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

bool DictCompiler::BuildReverseLookupDict(ReverseDb* db,
                                          uint32_t dict_file_checksum) {
  LOG(INFO) << "building reverse lookup db...";
  if (db->Exists())
    db->Remove();
  if (!db->Open())
    return false;
  // load syllable - word mapping from table
  Syllabary syllabary;
  if (!table_->Load() || !table_->GetSyllabary(&syllabary) || syllabary.empty())
    return false;
  typedef std::map<std::string, std::set<std::string> > ReverseLookupTable;
  ReverseLookupTable rev_table;
  int num_syllables = static_cast<int>(syllabary.size());
  for (int syllable_id = 0; syllable_id < num_syllables; ++syllable_id) {
    std::string syllable(table_->GetSyllableById(syllable_id));
    TableAccessor a(table_->QueryWords(syllable_id));
    while (!a.exhausted()) {
      std::string word(a.entry()->text.c_str());
      rev_table[word].insert(syllable);
      a.Next();
    }
  }
  // save reverse lookup dict
  BOOST_FOREACH(const ReverseLookupTable::value_type &v, rev_table) {
    std::string code_list(boost::algorithm::join(v.second, " "));
    db->Update(v.first, code_list);
  }
  db->MetaUpdate("/dict_file_checksum",
                 boost::lexical_cast<std::string>(dict_file_checksum));
  db->Close();
  return true;
}

}  // namespace rime
