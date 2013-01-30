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
#include <rime/dict/user_db.h>

namespace rime {

DictCompiler::DictCompiler(Dictionary *dictionary)
    : dict_name_(dictionary->name()),
      prism_(dictionary->prism()), table_(dictionary->table()) {
}

bool DictCompiler::Compile(const std::string &dict_file, const std::string &schema_file) {
  LOG(INFO) << "compiling:";
  uint32_t dict_file_checksum = dict_file.empty() ? 0 : Checksum(dict_file);
  uint32_t schema_file_checksum = schema_file.empty() ? 0 : Checksum(schema_file);
  LOG(INFO) << dict_file << " (" << dict_file_checksum << ")";
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
  TreeDb deprecated_db(dict_name_ + ".reverse.kct");
  if (deprecated_db.Exists()) {
    deprecated_db.Remove();
    LOG(INFO) << "removed deprecated db '" << deprecated_db.name() << "'.";
  }
  TreeDb db(dict_name_ + ".reverse.bin");
  if (db.Exists() && db.OpenReadOnly()) {
    std::string checksum;
    if (db.Fetch("\x01/dict_file_checksum", &checksum) &&
        boost::lexical_cast<uint32_t>(checksum) == dict_file_checksum) {
      rebuild_rev_lookup_dict = false;
    }
    db.Close();
  }
  if (rebuild_table && !BuildTable(dict_file, dict_file_checksum))
    return false;
  if (rebuild_prism && !BuildPrism(schema_file, dict_file_checksum, schema_file_checksum))
    return false;
  if (rebuild_rev_lookup_dict && !BuildReverseLookupDict(&db, dict_file_checksum))
    return false;
  // done!
  return true;
}

bool DictCompiler::BuildTable(const std::string &dict_file, uint32_t checksum) {
  LOG(INFO) << "building table...";

  DictSettings settings;
  if (!settings.LoadFromFile(dict_file))
    return false;
  LOG(INFO) << "dict name: " << settings.dict_name;
  LOG(INFO) << "dict version: " << settings.dict_version;
  
  EntryCollector collector;
  if (settings.use_preset_vocabulary) {
    collector.LoadPresetVocabulary(&settings);
  }
  collector.Collect(dict_file);
  // build table
  {
    std::map<std::string, int> syllable_to_id;
    int syllable_id = 0;
    BOOST_FOREACH(const std::string &s, collector.syllabary) {
      syllable_to_id[s] = syllable_id++;
    }
    Vocabulary vocabulary;
    BOOST_FOREACH(dictionary::RawDictEntry &r, collector.entries) {
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
    if (settings.sort_order != "original") {
      vocabulary.SortHomophones();
    }
    table_->Remove();
    if (!table_->Build(collector.syllabary, vocabulary, collector.num_entries, checksum) ||
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

bool DictCompiler::BuildReverseLookupDict(TreeDb *db, uint32_t dict_file_checksum) {
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
  db->Update("\x01/dict_file_checksum",
             boost::lexical_cast<std::string>(dict_file_checksum));
  db->Close();
  return true;
}

}  // namespace rime
