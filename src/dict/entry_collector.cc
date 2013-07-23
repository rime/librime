//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/dict/dict_settings.h>
#include <rime/dict/entry_collector.h>
#include <rime/dict/preset_vocabulary.h>

namespace rime {

EntryCollector::EntryCollector()
    : num_entries(0) {
}

EntryCollector::~EntryCollector() {
}

void EntryCollector::Configure(const DictSettings& settings) {
  if (settings.use_preset_vocabulary) {
    LoadPresetVocabulary(&settings);
  }
  if (settings.use_rule_based_encoder) {
    encoder.reset(new TableEncoder(this));
  }
  else {
    encoder.reset(new ScriptEncoder(this));
  }
}

void EntryCollector::Collect(const std::vector<std::string>& dict_files) {
  if (encoder && !dict_files.empty()) {
    encoder->LoadSettings(dict_files[0]);
  }
  BOOST_FOREACH(const std::string& dict_file, dict_files) {
    Collect(dict_file);
  }
  Finish();
}

void EntryCollector::LoadPresetVocabulary(const DictSettings* settings) {
  LOG(INFO) << "loading preset vocabulary.";
  preset_vocabulary.reset(PresetVocabulary::Create());
  if (preset_vocabulary && settings) {
    if (settings->max_phrase_length > 0)
      preset_vocabulary->set_max_phrase_length(settings->max_phrase_length);
    if (settings->min_phrase_weight > 0)
      preset_vocabulary->set_min_phrase_weight(settings->min_phrase_weight);
  }
}

void EntryCollector::Collect(const std::string &dict_file) {
  LOG(INFO) << "collecting entries from " << dict_file;
  // read column definitions
  DictSettings settings;
  if (!settings.LoadFromFile(dict_file)) {
    LOG(ERROR) << "missing dict settings.";
    return;
  }
  int text_column = settings.GetColumnIndex("text");
  int code_column = settings.GetColumnIndex("code");
  int weight_column = settings.GetColumnIndex("weight");
  int stem_column = settings.GetColumnIndex("stem");
  if (text_column == -1) {
    LOG(ERROR) << "missing text column definition.";
    return;
  }
  // read table
  std::ifstream fin(dict_file.c_str());
  std::string line;
  bool in_yaml_doc = true;
  bool enable_comment = true;
  while (getline(fin, line)) {
    boost::algorithm::trim_right(line);
    // skip yaml doc
    if (in_yaml_doc) {
      if (line == "...") in_yaml_doc = false;
      continue;
    }
    // skip empty lines and comments
    if (line.empty()) continue;
    if (enable_comment && line[0] == '#') {
      if (line == "# no comment") {
        // a "# no comment" line disables further comments
        enable_comment = false;
      }
      continue;
    }
    // read a dict entry
    std::vector<std::string> row;
    boost::algorithm::split(row, line,
                            boost::algorithm::is_any_of("\t"));
    int num_columns = static_cast<int>(row.size());
    if (num_columns <= text_column || row[text_column].empty()) {
      LOG(WARNING) << "Missing entry text at #" << num_entries << ".";
      continue;
    }
    std::string &word(row[text_column]);
    std::string code_str;
    std::string weight_str;
    std::string stem_str;
    if (code_column != -1 &&
        num_columns > code_column && !row[code_column].empty())
      code_str = row[code_column];
    if (weight_column != -1 &&
        num_columns > weight_column && !row[weight_column].empty())
      weight_str = row[weight_column];
    if (stem_column != -1 &&
        num_columns > stem_column && !row[stem_column].empty())
      stem_str = row[stem_column];
    // collect entry
    collection.insert(word);
    if (!code_str.empty()) {
      CreateEntry(word, code_str, weight_str);
    }
    else {
      encode_queue.push(std::make_pair(word, weight_str));
    }
    if (!stem_str.empty() && !code_str.empty()) {
      DLOG(INFO) << "add stem '" << word << "': "
                 << "[" << code_str << "] = [" << stem_str << "]";
      stems[word].insert(stem_str);
    }
  }
  LOG(INFO) << "Pass 1: total " << num_entries << " entries collected.";
  LOG(INFO) << "num unique syllables: " << syllabary.size();
  LOG(INFO) << "num of entries to encode: " << encode_queue.size();
}

void EntryCollector::Finish() {
  while (!encode_queue.empty()) {
    const std::string &phrase(encode_queue.front().first);
    const std::string &weight_str(encode_queue.front().second);
    if (!encoder->EncodePhrase(phrase, weight_str)) {
      LOG(ERROR) << "Encode failure: '" << phrase << "'.";
    }
    encode_queue.pop();
  }
  LOG(INFO) << "Pass 2: total " << num_entries << " entries collected.";
  if (preset_vocabulary) {
    preset_vocabulary->Reset();
    std::string phrase, weight_str;
    while (preset_vocabulary->GetNextEntry(&phrase, &weight_str)) {
      if (collection.find(phrase) != collection.end())
        continue;
      if (!encoder->EncodePhrase(phrase, weight_str)) {
        LOG(WARNING) << "Encode failure: '" << phrase << "'.";
      }
    }
  }
  LOG(INFO) << "Pass 3: total " << num_entries << " entries collected.";
}

void EntryCollector::CreateEntry(const std::string &word,
                                 const std::string &code_str,
                                 const std::string &weight_str) {
  RawDictEntry e;
  e.raw_code.FromString(code_str);
  e.text = word;
  e.weight = 0.0;
  bool scaled = boost::ends_with(weight_str, "%");
  if ((weight_str.empty() || scaled) && preset_vocabulary) {
    preset_vocabulary->GetWeightForEntry(e.text, &e.weight);
  }
  if (scaled) {
    double percentage = 100.0;
    try {
      percentage = boost::lexical_cast<double>(
          weight_str.substr(0, weight_str.length() - 1));
    }
    catch (...) {
      LOG(WARNING) << "invalid entry definition at #" << num_entries << ".";
      percentage = 100.0;
    }
    e.weight *= percentage / 100.0;
  }
  else if (!weight_str.empty()) {  // absolute weight
    try {
      e.weight = boost::lexical_cast<double>(weight_str);
    }
    catch (...) {
      LOG(WARNING) << "invalid entry definition at #" << num_entries << ".";
      e.weight = 0.0;
    }
  }
  // learn new syllables
  BOOST_FOREACH(const std::string &s, e.raw_code) {
    if (syllabary.find(s) == syllabary.end())
      syllabary.insert(s);
  }
  // learn new word
  bool is_word = (e.raw_code.size() == 1);
  if (is_word) {
    if (words[e.text].find(code_str) != words[e.text].end()) {
      LOG(WARNING) << "duplicate word definition '"
                   << e.text << "': [" << code_str << "].";
      return;
    }
    words[e.text][code_str] += e.weight;
    total_weight[e.text] += e.weight;
  }
  entries.push_back(e);
  ++num_entries;
}

bool EntryCollector::TranslateWord(const std::string& word,
                                   std::vector<std::string>* result) {
  StemMap::const_iterator s = stems.find(word);
  if (s != stems.end()) {
    BOOST_FOREACH(const std::string& stem, s->second) {
      result->push_back(stem);
    }
    return true;
  }
  WordMap::const_iterator w = words.find(word);
  if (w != words.end()) {
    BOOST_FOREACH(const WeightMap::value_type& v, w->second) {
      const double kMinimalWeight = 0.05;  // 5%
      double min_weight = total_weight[word] * kMinimalWeight;
      if (v.second < min_weight)
        continue;
      result->push_back(v.first);
    }
    return true;
  }
  return false;
}

void EntryCollector::Dump(const std::string& file_name) const {
  std::ofstream out(file_name.c_str());
  out << "# syllabary:" << std::endl;
  BOOST_FOREACH(const std::string& syllable, syllabary) {
    out << "# - " << syllable << std::endl;
  }
  out << std::endl;
  BOOST_FOREACH(const RawDictEntry &e, entries) {
    out << e.text << '\t'
        << e.raw_code.ToString() << '\t'
        << e.weight << std::endl;
  }
  out.close();
}

}  // namespace rime
