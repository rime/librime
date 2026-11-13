//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <fstream>
#include <utility>
#include <boost/algorithm/string.hpp>
#include <rime/algo/strings.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/entry_collector.h>
#include <rime/dict/preset_vocabulary.h>

namespace rime {

namespace {

struct DictRow {
  std::string_view text;
  std::string_view code;
  std::string_view weight;
  std::string_view stem;
  int num_columns = 0;
};

// Splits a tab-separated row into the columns we are interested in without
// copying the whole line. The returned views are valid until the input line is
// modified or destroyed.
DictRow ParseDictRow(std::string_view line,
                     int text_column,
                     int code_column,
                     int weight_column,
                     int stem_column) {
  DictRow row;
  int column = 0;
  size_t start = 0;
  while (true) {
    size_t end = line.find('\t', start);
    std::string_view field = line.substr(start, end == std::string_view::npos
                                                    ? std::string_view::npos
                                                    : end - start);
    if (column == text_column)
      row.text = field;
    if (column == code_column)
      row.code = field;
    if (column == weight_column)
      row.weight = field;
    if (column == stem_column)
      row.stem = field;
    ++column;
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  row.num_columns = column;
  return row;
}

// A code is single-syllable when it has exactly one non-empty token.
bool IsSingleSyllable(std::string_view code) {
  size_t start = code.find_first_not_of(' ');
  if (start == std::string_view::npos)
    return false;
  size_t end = code.find(' ', start);
  return end == std::string_view::npos ||
         code.find_first_not_of(' ', end) == std::string_view::npos;
}

}  // namespace

static bool has_significant_weight(double weight, double total_weight) {
  constexpr double kMinSignificantWeightRatio = 0.05;  // 5%
  // Keep the original comparison semantics, including for NaN values.
  return !(weight < total_weight * kMinSignificantWeightRatio);
}

EntryCollector::EntryCollector() {}

EntryCollector::EntryCollector(Syllabary&& fixed_syllabary)
    : syllabary(std::move(fixed_syllabary)), build_syllabary(false) {}

EntryCollector::~EntryCollector() {}

void EntryCollector::Configure(DictSettings* settings) {
  if (settings->use_preset_vocabulary()) {
    LoadPresetVocabulary(settings);
  }

  if (settings->use_rule_based_encoder()) {
    encoder.reset(new TableEncoder(this));
  } else {
    encoder.reset(new ScriptEncoder(this));
  }
  encoder->LoadSettings(settings);
}

void EntryCollector::Collect(const vector<path>& dict_files) {
  for (const path& dict_file : dict_files) {
    Collect(dict_file);
  }
  Finish();
}

void EntryCollector::LoadPresetVocabulary(DictSettings* settings) {
  auto vocabulary = settings->vocabulary();
  LOG(INFO) << "loading preset vocabulary: " << vocabulary;
  preset_vocabulary.reset(new PresetVocabulary(vocabulary));
  if (preset_vocabulary) {
    if (settings->max_phrase_length() > 0)
      preset_vocabulary->set_max_phrase_length(settings->max_phrase_length());
    if (settings->min_phrase_weight() > 0)
      preset_vocabulary->set_min_phrase_weight(settings->min_phrase_weight());
  }
}

void EntryCollector::Collect(const path& dict_file) {
  LOG(INFO) << "collecting entries from " << dict_file;
  current_dict_file = dict_file.to_utf8_string();
  line_number = 0;
  // read table
  std::ifstream fin(dict_file.c_str());
  DictSettings settings;
  if (!settings.LoadDictHeader(fin)) {
    LOG(ERROR) << "missing dict settings.";
    return;
  }
  // column definitions
  int text_column = settings.GetColumnIndex("text");
  int code_column = settings.GetColumnIndex("code");
  int weight_column = settings.GetColumnIndex("weight");
  int stem_column = settings.GetColumnIndex("stem");
  if (text_column == -1) {
    LOG(ERROR) << "missing text column definition in file: " << dict_file
               << ".";
    return;
  }
  bool enable_comment = true;
  string line;
  while (getline(fin, line)) {
    boost::algorithm::trim_right(line);
    line_number++;
    // skip empty lines and comments
    if (line.empty())
      continue;
    if (enable_comment && line[0] == '#') {
      if (line == "# no comment") {
        // a "# no comment" line disables further comments
        enable_comment = false;
      }
      continue;
    }
    // read a dict entry
    DictRow row = ParseDictRow(line, text_column, code_column, weight_column,
                               stem_column);
    if (row.num_columns <= text_column || row.text.empty()) {
      LOG(WARNING) << "Missing entry text at #" << num_entries
                   << ", line: " << line_number
                   << " of file: " << current_dict_file << ".";
      continue;
    }
    // collect entry
    if (preset_vocabulary)
      collection.emplace(row.text);
    if (!row.code.empty()) {
      CreateEntryImpl(row.text, row.code, row.weight);
    } else {
      encode_queue.emplace(row.text, row.weight);
    }
    if (!row.stem.empty() && !row.code.empty()) {
      DLOG(INFO) << "add stem '" << row.text << "': "
                 << "[" << row.code << "] = [" << row.stem << "]";
      stems[string(row.text)].insert(string(row.stem));
    }
  }
  fin.close();
  LOG(INFO) << "Pass 1: total " << num_entries << " entries collected.";
  LOG(INFO) << "num unique syllables: " << syllabary.size();
  LOG(INFO) << "num of entries to encode: " << encode_queue.size();
}

void EntryCollector::Finish() {
  DeduplicateEntries();
  while (!encode_queue.empty()) {
    const auto& phrase(encode_queue.front().first);
    const auto& weight_str(encode_queue.front().second);
    if (!encoder->EncodePhrase(phrase, weight_str)) {
      LOG(ERROR) << "Encode failure: '" << phrase << "'.";
    }
    encode_queue.pop();
  }
  LOG(INFO) << "Pass 2: total " << num_entries << " entries collected.";
  if (preset_vocabulary) {
    preset_vocabulary->Reset();
    string phrase, weight_str;
    while (preset_vocabulary->GetNextEntry(&phrase, &weight_str)) {
      if (collection.find(phrase) != collection.end())
        continue;
      if (!encoder->EncodePhrase(phrase, weight_str)) {
        LOG(WARNING) << "Encode failure: '" << phrase << "'.";
      }
    }
    // Finish collection before releasing preset weights used by CreateEntry().
    preset_vocabulary.reset();
  }
  decltype(collection)().swap(collection);
  decltype(words)().swap(words);
  LOG(INFO) << "Pass 3: total " << num_entries << " entries collected.";
}

void EntryCollector::CreateEntry(const string& word,
                                 const string& code_str,
                                 const string& weight_str) {
  CreateEntryImpl(word, code_str, weight_str);
}

void EntryCollector::CreateEntryImpl(std::string_view word,
                                     std::string_view code_str,
                                     std::string_view weight_str) {
  RawCode raw_code;
  raw_code.FromStringView(code_str);
  RawDictEntry e;
  e.text.assign(word);
  e.code.assign(code_str);
  e.weight = 0.0;
  bool scaled = !weight_str.empty() && weight_str.back() == '%';
  if ((weight_str.empty() || scaled) && preset_vocabulary) {
    preset_vocabulary->GetWeightForEntry(e.text, &e.weight);
  }
  if (scaled) {
    double percentage = 100.0;
    try {
      percentage =
          std::stod(string(weight_str.substr(0, weight_str.length() - 1)));
    } catch (...) {
      LOG(WARNING) << "invalid entry definition at #" << num_entries
                   << ", line: " << line_number
                   << " of file: " << current_dict_file << ".";
      percentage = 100.0;
    }
    e.weight *= percentage / 100.0;
  } else if (!weight_str.empty()) {  // absolute weight
    try {
      e.weight = std::stod(string(weight_str));
    } catch (...) {
      LOG(WARNING) << "invalid entry definition at #" << num_entries
                   << ", line: " << line_number
                   << " of file: " << current_dict_file << ".";
      e.weight = 0.0;
    }
  }
  // learn new syllables, or check if syllables are in the fixed syllabary.
  for (const string& s : raw_code) {
    if (syllabary.find(s) == syllabary.end()) {
      if (build_syllabary) {
        syllabary.insert(s);
      } else {
        LOG(ERROR) << "dropping entry '" << e.text
                   << "' with invalid syllable: " << s;
        return;
      }
    }
  }
  // Keep an already-built word map up to date; entries added while encoding
  // preset phrases must remain visible to subsequent TranslateWord() calls.
  if (words_built_ && raw_code.size() == 1) {
    auto& word = words[e.text];
    if (!word.has_encoding) {
      word.has_encoding = true;
      word.code = e.code;
      word.weight = e.weight;
    } else {
      bool duplicate =
          e.code == word.code ||
          std::any_of(word.alternative_encodings.begin(),
                      word.alternative_encodings.end(),
                      [&](const auto& p) { return p.first == e.code; });
      if (duplicate) {
        LOG(WARNING) << "duplicate word definition '" << e.text << "': ["
                     << e.code << "].";
        return;
      }
      word.alternative_encodings.emplace_back(e.code, e.weight);
    }
  }
  total_text_bytes += e.text.size();
  entries.emplace_back(std::move(e));
  ++num_entries;
  deduplicated_ = false;
}

void EntryCollector::DeduplicateEntries() {
  if (deduplicated_)
    return;
  deduplicated_ = true;
  vector<size_t> indices;
  indices.reserve(entries.size());
  for (size_t i = 0; i < entries.size(); ++i) {
    if (IsSingleSyllable(entries[i].code))
      indices.push_back(i);
  }
  if (indices.size() > 1) {
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
      const auto& ea = entries[a];
      const auto& eb = entries[b];
      if (ea.text != eb.text)
        return ea.text < eb.text;
      if (ea.code != eb.code)
        return ea.code < eb.code;
      return a < b;  // keep the earliest occurrence
    });
    vector<char> remove(entries.size(), 0);
    size_t removed = 0;
    for (size_t k = 1; k < indices.size(); ++k) {
      const auto& previous = entries[indices[k - 1]];
      const auto& current = entries[indices[k]];
      if (previous.text == current.text && previous.code == current.code) {
        remove[indices[k]] = 1;
        ++removed;
        LOG(WARNING) << "duplicate word definition '" << current.text << "': ["
                     << current.code << "].";
      }
    }
    if (removed) {
      size_t write = 0;
      for (size_t i = 0; i < entries.size(); ++i) {
        if (remove[i])
          continue;
        if (write != i)
          entries[write] = std::move(entries[i]);
        ++write;
      }
      entries.resize(write);
    }
  }
  num_entries = entries.size();
}

void EntryCollector::EnsureWords() {
  if (words_built_)
    return;
  DeduplicateEntries();
  words.clear();
  BuildWords();
  words_built_ = true;
}

void EntryCollector::BuildWords() {
  words.reserve(entries.size());
  for (const auto& e : entries) {
    if (!IsSingleSyllable(e.code))
      continue;
    auto& word = words[e.text];
    if (!word.has_encoding) {
      word.has_encoding = true;
      word.code = e.code;
      word.weight = e.weight;
    } else {
      bool duplicate =
          e.code == word.code ||
          std::any_of(word.alternative_encodings.begin(),
                      word.alternative_encodings.end(),
                      [&](const auto& p) { return p.first == e.code; });
      if (duplicate)
        continue;
      word.alternative_encodings.emplace_back(e.code, e.weight);
    }
  }
}

bool EntryCollector::TranslateWord(const string& word, vector<string>* result) {
  const auto& s = stems.find(word);
  if (s != stems.end()) {
    for (const string& stem : s->second) {
      result->push_back(stem);
    }
    return true;
  }
  EnsureWords();
  const auto& w = words.find(word);
  if (w != words.end()) {
    const auto& word_entries = w->second;
    if (word_entries.alternative_encodings.empty()) {
      if (has_significant_weight(word_entries.weight, word_entries.weight))
        result->push_back(word_entries.code);
      return true;
    }
    struct Encoding {
      std::string_view code;
      double weight;
    };
    vector<Encoding> encodings;
    encodings.reserve(1 + word_entries.alternative_encodings.size());
    encodings.push_back({word_entries.code, word_entries.weight});
    double total_weight = word_entries.weight;
    for (const auto& extra : word_entries.alternative_encodings) {
      encodings.push_back({extra.first, extra.second});
      total_weight += extra.second;
    }
    std::sort(encodings.begin(), encodings.end(),
              [](const auto& a, const auto& b) { return a.code < b.code; });
    for (const auto& v : encodings) {
      if (!has_significant_weight(v.weight, total_weight))
        continue;
      result->push_back(string(v.code));
    }
    return true;
  }
  return false;
}

void EntryCollector::Dump(const path& file_path) const {
  std::ofstream out(file_path.c_str());
  out << "# syllabary:" << std::endl;
  for (const string& syllable : syllabary) {
    out << "# - " << syllable << std::endl;
  }
  out << std::endl;
  for (const auto& e : entries) {
    RawCode raw_code;
    raw_code.FromStringView(e.code);
    out << e.text << '\t' << raw_code.ToString() << '\t' << e.weight
        << std::endl;
  }
  out.close();
}

}  // namespace rime
