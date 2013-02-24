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

void EntryCollector::LoadPresetVocabulary(DictSettings* settings) {
    preset_vocabulary.reset(PresetVocabulary::Create());
    if (preset_vocabulary && settings) {
      if (settings->max_phrase_length > 0)
        preset_vocabulary->set_max_phrase_length(settings->max_phrase_length);
      if (settings->min_phrase_weight > 0)
        preset_vocabulary->set_min_phrase_weight(settings->min_phrase_weight);
    }
}

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
      LOG(WARNING) << "Missing entry text at #" << num_entries << ".";
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
  LOG(INFO) << "Pass 1: " << num_entries << " entries collected.";
  LOG(INFO) << "num unique syllables: " << syllabary.size();
  LOG(INFO) << "num of entries to encode: " << encode_queue.size();
  dictionary::RawCode code;
  while (!encode_queue.empty()) {
    const std::string &phrase(encode_queue.front().first);
    const std::string &weight_str(encode_queue.front().second);
    code.clear();
    if (!Encode(phrase, weight_str, 0, &code)) {
      LOG(ERROR) << "Encode failure: '" << phrase << "'.";
    }
    encode_queue.pop();
  }
  LOG(INFO) << "Pass 2: " << num_entries << " entries collected.";
  if (preset_vocabulary) {
    preset_vocabulary->Reset();
    std::string phrase, weight_str;
    while (preset_vocabulary->GetNextEntry(&phrase, &weight_str)) {
      if (collection.find(phrase) != collection.end())
        continue;
      code.clear();
      if (!Encode(phrase, weight_str, 0, &code)) {
        LOG(WARNING) << "Encode failure: '" << phrase << "'.";
      }
    }
  }
  LOG(INFO) << "Pass 3: " << num_entries << " entries collected.";
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
  e.raw_code.FromString(code_str);
  // learn new syllables
  BOOST_FOREACH(const std::string &s, e.raw_code) {
    if (syllabary.find(s) == syllabary.end())
      syllabary.insert(s);
  }
  // learn new word
  if (e.raw_code.size() == 1) {
    if (words[e.text].find(code_str) != words[e.text].end()) {
      LOG(WARNING) << "duplicate word definition '" << e.text << "': [" << code_str << "].";
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

void EntryCollector::Dump(const std::string& file_name) const {
  std::ofstream out(file_name.c_str());
  out << "# syllabary:" << std::endl;
  BOOST_FOREACH(const std::string& syllable, syllabary) {
    out << "# - " << syllable << std::endl;
  }
  out << std::endl;
  BOOST_FOREACH(const dictionary::RawDictEntry &e, entries) {
    out << e.text << '\t'
        << e.raw_code.ToString() << '\t'
        << e.weight << std::endl;
  }
  out.close();
}

}  // namespace rime
