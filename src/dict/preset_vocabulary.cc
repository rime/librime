// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <utf8.h>
#include <rime/service.h>
#include <rime/dict/preset_vocabulary.h>

namespace rime {

PresetVocabulary::PresetVocabulary(const shared_ptr<kyotocabinet::TreeDB>& db)
    : db_(db), cursor_(db->cursor()),
      max_phrase_length_(0), min_phrase_weight_(0.0) {
}

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

}  // namespace rime
