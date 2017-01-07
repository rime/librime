//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <utf8.h>
#include <rime/service.h>
#include <rime/dict/preset_vocabulary.h>
#include <rime/dict/text_db.h>

namespace rime {

struct VocabularyDb : public TextDb {
  explicit VocabularyDb(const string& path);
  an<DbAccessor> cursor;
  static const TextFormat format;
};

VocabularyDb::VocabularyDb(const string& path)
    : TextDb(path, "vocabulary", VocabularyDb::format) {
}

static bool rime_vocabulary_entry_parser(const Tsv& row,
                                         string* key,
                                         string* value) {
  if (row.size() < 1 || row[0].empty()) {
    return false;
  }
  *key = row[0];
  *value = row.size() > 1 ? row[1] : "0";
  return true;
}

static bool rime_vocabulary_entry_formatter(const string& key,
                                            const string& value,
                                            Tsv* tsv) {
  //Tsv& row(*tsv);
  //row.push_back(key);
  //row.push_back(value);
  return true;
}

const TextFormat VocabularyDb::format = {
  rime_vocabulary_entry_parser,
  rime_vocabulary_entry_formatter,
  "Rime vocabulary",
};

string PresetVocabulary::DictFilePath() {
  auto& deployer(Service::instance().deployer());
  boost::filesystem::path path(deployer.user_data_dir);
  path /= "essay.txt";
  if (!boost::filesystem::exists(path)) {
    path = deployer.shared_data_dir;
    path /= "essay.txt";
  }
  return path.string();
}

PresetVocabulary::PresetVocabulary() {
  db_.reset(new VocabularyDb(DictFilePath()));
  if (db_ && db_->OpenReadOnly()) {
    db_->cursor = db_->QueryAll();
  }
}

PresetVocabulary::~PresetVocabulary() {
  if (db_)
    db_->Close();
}

bool PresetVocabulary::GetWeightForEntry(const string &key, double *weight) {
  string weight_str;
  if (!db_ || !db_->Fetch(key, &weight_str))
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
  if (db_ && db_->cursor)
    db_->cursor->Reset();
}

bool PresetVocabulary::GetNextEntry(string *key, string *value) {
  if (!db_ || !db_->cursor)
    return false;
  bool got = false;
  do {
    got = db_->cursor->GetNextRecord(key, value);
  }
  while (got && !IsQualifiedPhrase(*key, *value));
  return got;
}

bool PresetVocabulary::IsQualifiedPhrase(const string& phrase,
                                         const string& weight_str) {
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

}  // namespace rime
