//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
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
#include <rime/service.h>
#include <rime/dict/preset_vocabulary.h>

namespace rime {

struct VocabularyDb {
  kyotocabinet::TreeDB kcdb;
  scoped_ptr<kyotocabinet::DB::Cursor> kcursor;

  VocabularyDb();
};

VocabularyDb::VocabularyDb()
    : kcdb(), kcursor(kcdb.cursor()) {
  //kcdb->tune_options(kyotocabinet::TreeDB::TLINEAR |
  //                        kyotocabinet::TreeDB::TCOMPRESS);
  //kcdb->tune_buckets(30LL * 1000);
  kcdb.tune_defrag(8);
  kcdb.tune_page(32768);
}

std::string PresetVocabulary::DictFilePath() {
  boost::filesystem::path path(Service::instance().deployer().shared_data_dir);
  path /= "essay.kct";
  return path.string();
}

PresetVocabulary::PresetVocabulary()
    : max_phrase_length_(0), min_phrase_weight_(0.0) {
  db_.reset(new VocabularyDb);
  if (!db_) return;
  if (!db_->kcdb.open(DictFilePath(), kyotocabinet::TreeDB::OREADER)) {
    db_.reset();
  }
}

PresetVocabulary::~PresetVocabulary() {
}

bool PresetVocabulary::GetWeightForEntry(const std::string &key, double *weight) {
  std::string weight_str;
  if (!db_ || !db_->kcdb.get(key, &weight_str))
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
  if (db_ && db_->kcursor)
    db_->kcursor->jump();
}

bool PresetVocabulary::GetNextEntry(std::string *key, std::string *value) {
  if (!db_ || !db_->kcursor)
    return false;
  bool got = false;
  do {
    got = db_->kcursor->get(key, value, true);
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

}  // namespace rime
