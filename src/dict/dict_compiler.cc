// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4351)
#endif
#include <kchashdb.h>
#if defined(_MSC_VER)
#pragma warning(default: 4351)
#pragma warning(default: 4244)
#endif
#include <utf8.h>
#include <yaml-cpp/yaml.h>
#include <rime/service.h>
#include <rime/algo/algebra.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/user_db.h>

namespace rime {

namespace dictionary {

uint32_t checksum(const std::string &file_name) {
  std::ifstream fin(file_name.c_str());
  std::string file_content((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
  boost::crc_32_type crc_32;
  crc_32.process_bytes(file_content.data(), file_content.length());
  return crc_32.checksum();
}

}  // namespace dictionary

class PresetVocabulary {
 public:
  static PresetVocabulary *Create();
  // random access
  bool GetWeightForEntry(const std::string &key, double *weight);
  // traversing
  void Reset();
  bool GetNextEntry(std::string *key, std::string *value);
  bool IsQualifiedPhrase(const std::string& phrase,
                         const std::string& weight_str);
  
  void set_max_phrase_length(int length) { max_phrase_length_ = length; }
  void set_min_phrase_weight(double weight) { min_phrase_weight_ = weight; }

 protected:
  PresetVocabulary(const shared_ptr<kyotocabinet::TreeDB>& db)
      : db_(db), cursor_(db->cursor()),
        max_phrase_length_(0), min_phrase_weight_(0.0) {}
  
  shared_ptr<kyotocabinet::TreeDB> db_;
  scoped_ptr<kyotocabinet::DB::Cursor> cursor_;
  int max_phrase_length_;
  double min_phrase_weight_;
};

PresetVocabulary* PresetVocabulary::Create() {
  boost::filesystem::path path(Service::instance().deployer().shared_data_dir);
  path /= "essay.kct";
  shared_ptr<kyotocabinet::TreeDB> db(new kyotocabinet::TreeDB);
  if (!db) return NULL;
  //db->tune_options(kyotocabinet::TreeDB::TLINEAR | kyotocabinet::TreeDB::TCOMPRESS);
  //db->tune_buckets(30LL * 1000);
  db->tune_defrag(8);
  db->tune_page(32768);
  if (!db->open(path.string(), kyotocabinet::TreeDB::OREADER)) {
    return NULL;
  }
  return new PresetVocabulary(db);
}

bool PresetVocabulary::GetWeightForEntry(const std::string &key, double *weight) {
  std::string weight_str;
  if (!db_ || !db_->get(key, &weight_str))
    return false;
  try {
    *weight = boost::lexical_cast<double>(weight_str);
  }
  catch (...) {
    return false;
  }
  return true;
}

void PresetVocabulary::Reset() {
  if (cursor_)
    cursor_->jump();
}

bool PresetVocabulary::GetNextEntry(std::string *key, std::string *value) {
  if (!cursor_) return false;
  bool got = false;
  do {
    got = cursor_->get(key, value, true);
  }
  while (got && !IsQualifiedPhrase(*key, *value));
  return got;
}

bool PresetVocabulary::IsQualifiedPhrase(const std::string& phrase,
                                         const std::string& weight_str) {
  if (max_phrase_length_ > 0) {
    size_t length = utf8::unchecked::distance(phrase.c_str(),
                                              phrase.c_str() + phrase.length());
    if (static_cast<int>(length) > max_phrase_length_)
      return false;
  }
  if (min_phrase_weight_ > 0.0) {
    double weight = boost::lexical_cast<double>(weight_str);
    if (weight < min_phrase_weight_)
      return false;
  }
  return true;
}

// EntryCollector

struct EntryCollector {
  scoped_ptr<PresetVocabulary> preset_vocabulary;
  Syllabary syllabary;
  std::vector<dictionary::RawDictEntry> entries;
  size_t num_entries;
  std::queue<std::pair<std::string, std::string> > encode_queue;
  typedef std::map<std::string, double> WeightMap;
  std::map<std::string, WeightMap> words;
  WeightMap total_weight_for_word;
  std::set<std::string> collection;

  void Collect(const std::string &dict_file);
  void CreateEntry(const std::string &word,
                   const std::string &code_str,
                   const std::string &weight_str);
  bool Encode(const std::string &phrase, const std::string &weight_str,
              size_t start_pos, dictionary::RawCode *code);
};

void EntryCollector::Collect(const std::string &dict_file) {
  std::ifstream fin(dict_file.c_str());
  std::string line;
  bool in_yaml_doc = true;
  while (getline(fin, line)) {
    boost::algorithm::trim_right(line);
    // skip yaml doc
    if (in_yaml_doc) {
      if (line == "...") in_yaml_doc = false;
      continue;
    }
    // skip empty lines and comments
    if (line.empty() || line[0] == '#') continue;
    // read a dict entry
    std::vector<std::string> row;
    boost::algorithm::split(row, line,
                            boost::algorithm::is_any_of("\t"));
    if (row.size() == 0 || row[0].empty()) {
      EZLOGGERPRINT("Missing entry text at #%d.", num_entries);
      continue;
    }
    std::string &word(row[0]);
    std::string code_str;
    std::string weight_str;
    if (row.size() > 1 && !row[1].empty())
      code_str = row[1];
    if (row.size() > 2 && !row[2].empty())
      weight_str = row[2];
    collection.insert(word);
    if (!code_str.empty()) {
      CreateEntry(word, code_str, weight_str);
    }
    else {
      encode_queue.push(std::make_pair(word, weight_str));
    }
  }
  EZLOGGERPRINT("Pass 1: %d entries collected.", num_entries);
  EZLOGGERVAR(syllabary.size());
  EZLOGGERVAR(encode_queue.size());
  dictionary::RawCode code;
  while (!encode_queue.empty()) {
    const std::string &phrase(encode_queue.front().first);
    const std::string &weight_str(encode_queue.front().second);
    code.clear();
    if (!Encode(phrase, weight_str, 0, &code)) {
      EZLOGGERPRINT("Encode failure: '%s'.", phrase.c_str());
    }
    encode_queue.pop();
  }
  EZLOGGERPRINT("Pass 2: %d entries collected.", num_entries);
  if (preset_vocabulary) {
    preset_vocabulary->Reset();
    std::string phrase, weight_str;
    while (preset_vocabulary->GetNextEntry(&phrase, &weight_str)) {
      if (collection.find(phrase) != collection.end())
        continue;
      code.clear();
      if (!Encode(phrase, weight_str, 0, &code)) {
        EZLOGGERPRINT("Encode failure: '%s'.", phrase.c_str());
      }
    }
  }
  EZLOGGERPRINT("Pass 3: %d entries collected.", num_entries);
}

void EntryCollector::CreateEntry(const std::string &word,
                                 const std::string &code_str,
                                 const std::string &weight_str) {
  dictionary::RawDictEntry e;
  e.text = word;
  e.weight = 0.0;
  bool scaled = boost::ends_with(weight_str, "%");
  if ((weight_str.empty() || scaled) && preset_vocabulary) {
    preset_vocabulary->GetWeightForEntry(e.text, &e.weight);
  }
  if (scaled) {
    double percentage = 100.0;
    try {
      percentage = boost::lexical_cast<double>(weight_str.substr(0, weight_str.length() - 1));
    }
    catch (...) {
      EZLOGGERPRINT("Warning: invalid entry definition at #%d.", num_entries);
      percentage = 100.0;
    }
    e.weight *= percentage / 100.0;
  }
  else if (!weight_str.empty()) {  // absolute weight
    try {
      e.weight = boost::lexical_cast<double>(weight_str);
    }
    catch (...) {
      EZLOGGERPRINT("Warning: invalid entry definition at #%d.", num_entries);
      e.weight = 0.0;
    }
  }
  e.raw_code.FromString(code_str);
  // learn new syllables
  BOOST_FOREACH(const std::string &s, e.raw_code) {
    if (syllabary.find(s) == syllabary.end())
      syllabary.insert(s);
  }
  // learn new word
  if (e.raw_code.size() == 1) {
    if (words[e.text].find(code_str) != words[e.text].end()) {
      EZLOGGERPRINT("Warning: duplicate word definition '%s' : [%s].",
                    e.text.c_str(), code_str.c_str());
    }
    words[e.text][code_str] += e.weight;
    total_weight_for_word[e.text] += e.weight;
  }
  entries.push_back(e);
  ++num_entries;
}

bool EntryCollector::Encode(const std::string &phrase, const std::string &weight_str,
                            size_t start_pos, dictionary::RawCode *code) {
  const double kMinimalWeightProportionForWordMaking = 0.05;
  if (start_pos == phrase.length()) {
    CreateEntry(phrase, code->ToString(), weight_str);
    return true;
  }
  bool ret = false;
  for (size_t k = phrase.length() - start_pos; k > 0; --k) {
    std::string w(phrase.substr(start_pos, k));
    if (words.find(w) != words.end()) {
      BOOST_FOREACH(const WeightMap::value_type &v, words[w]) {
        double min_weight = total_weight_for_word[w] * kMinimalWeightProportionForWordMaking;
        if (v.second < min_weight)
          continue;
        code->push_back(v.first);
        bool ok = Encode(phrase, weight_str, start_pos + k, code);
        ret = ret || ok;
        code->pop_back();
      }
    }
  }
  return ret;
}

// DictCompiler

DictCompiler::DictCompiler(Dictionary *dictionary)
    : dict_name_(dictionary->name()),
      prism_(dictionary->prism()), table_(dictionary->table()) {
}

bool DictCompiler::Compile(const std::string &dict_file, const std::string &schema_file) {
  EZLOGGERFUNCTRACKER;
  uint32_t dict_file_checksum = dict_file.empty() ? 0 : dictionary::checksum(dict_file);
  uint32_t schema_file_checksum = schema_file.empty() ? 0 : dictionary::checksum(schema_file);
  EZLOGGERVAR(dict_file_checksum);
  EZLOGGERVAR(schema_file_checksum);
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
    EZLOGGERPRINT("removed deprecated db '%s'.", deprecated_db.name().c_str());
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
  EZLOGGERPRINT("building table...");
  YAML::Node doc;
  {
    std::ifstream fin(dict_file.c_str());
    YAML::Parser parser(fin);
    if (!parser.GetNextDocument(doc)) {
      EZLOGGERPRINT("Error parsing yaml doc in '%s'.", dict_file.c_str());
      return false;
    }
  }
  if (doc.Type() != YAML::NodeType::Map) {
    EZLOGGERPRINT("Error: invalid yaml doc in '%s'.", dict_file.c_str());
    return false;
  }
  std::string dict_name;
  std::string dict_version;
  std::string sort_order;
  bool use_preset_vocabulary = false;
  int max_phrase_length = 0;
  double min_phrase_weight = 0;
  {
    const YAML::Node *name_node = doc.FindValue("name");
    const YAML::Node *version_node = doc.FindValue("version");
    const YAML::Node *sort_order_node = doc.FindValue("sort");
    const YAML::Node *use_preset_vocabulary_node = doc.FindValue("use_preset_vocabulary");
    const YAML::Node *max_phrase_length_node = doc.FindValue("max_phrase_length");
    const YAML::Node *min_phrase_weight_node = doc.FindValue("min_phrase_weight");
    if (!name_node || !version_node) {
      EZLOGGERPRINT("Error: incomplete dict info in '%s'.", dict_file.c_str());
      return false;
    }
    *name_node >> dict_name;
    *version_node >> dict_version;
    if (sort_order_node) {
      *sort_order_node >> sort_order;
    }
    if (use_preset_vocabulary_node) {
      *use_preset_vocabulary_node >> use_preset_vocabulary;
      if (max_phrase_length_node)
        *max_phrase_length_node >> max_phrase_length;
      if (min_phrase_weight_node)
        *min_phrase_weight_node >> min_phrase_weight;
    }
  }
  EZLOGGERVAR(dict_name);
  EZLOGGERVAR(dict_version);
  
  EntryCollector collector;
  collector.num_entries = 0;
  if (use_preset_vocabulary) {
    collector.preset_vocabulary.reset(PresetVocabulary::Create());
    if (max_phrase_length > 0)
      collector.preset_vocabulary->set_max_phrase_length(max_phrase_length);
    if (min_phrase_weight > 0)
      collector.preset_vocabulary->set_min_phrase_weight(min_phrase_weight);
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
        EZLOGGERPRINT("Error locating entries in vocabulary.");
        continue;
      }
      shared_ptr<DictEntry> e = make_shared<DictEntry>();
      e->code.swap(code);
      e->text.swap(r.text);
      e->weight = r.weight;
      ls->push_back(e);
    }
    if (sort_order != "original") {
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
  EZLOGGERPRINT("building prism...");
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
  EZLOGGERPRINT("building reverse lookup db...");
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
