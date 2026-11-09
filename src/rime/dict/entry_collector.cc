//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-27 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <array>
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

size_t CountLines(const path& dict_file) {
  std::ifstream fin(dict_file.c_str(), std::ios::binary);
  if (!fin)
    return 0;
  std::array<char, 1 << 16> buffer;
  size_t count = 0;
  while (fin.read(buffer.data(), buffer.size()) || fin.gcount()) {
    count += static_cast<size_t>(
        std::count(buffer.data(), buffer.data() + fin.gcount(), '\n'));
  }
  return count;
}

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

}  // namespace

static bool HasMinimumWeight(double weight, double total_weight) {
  constexpr double kMinimalWeight = 0.05;  // 5%
  // Keep the original comparison semantics, including for NaN values.
  return !(weight < total_weight * kMinimalWeight);
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
  // Reserve once for all imports to avoid repeated growth and relocation.
  size_t line_count = 0;
  for (const path& dict_file : dict_files) {
    line_count += CountLines(dict_file);
  }
  if (line_count) {
    entries.reserve(entries.size() + line_count);
    words.reserve(words.size() + line_count);
  }
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
  RawDictEntry e;
  e.raw_code.FromStringView(code_str);
  e.text.assign(word);
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
  for (const string& s : e.raw_code) {
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
  // learn new word
  bool is_word = (e.raw_code.size() == 1);
  if (is_word) {
    auto& word = words[e.text];
    if (!word.has_encoding) {
      word.has_encoding = true;
      word.code.assign(code_str);
      word.weight = e.weight;
    } else {
      bool duplicate =
          std::string_view(word.code) == code_str ||
          std::any_of(word.alternative_encodings.begin(),
                      word.alternative_encodings.end(), [&](const auto& p) {
                        return std::string_view(p.first) == code_str;
                      });
      if (duplicate) {
        LOG(WARNING) << "duplicate word definition '" << e.text << "': ["
                     << code_str << "].";
        return;
      }
      word.alternative_encodings.emplace_back(string(code_str), e.weight);
    }
  }
  entries.emplace_back(std::move(e));
  ++num_entries;
}

bool EntryCollector::TranslateWord(const string& word, vector<string>* result) {
  const auto& s = stems.find(word);
  if (s != stems.end()) {
    for (const string& stem : s->second) {
      result->push_back(stem);
    }
    return true;
  }
  const auto& w = words.find(word);
  if (w != words.end()) {
    const auto& word_entries = w->second;
    if (word_entries.alternative_encodings.empty()) {
      if (HasMinimumWeight(word_entries.weight, word_entries.weight))
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
      if (!HasMinimumWeight(v.weight, total_weight))
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
    out << e.text << '\t' << e.raw_code.ToString() << '\t' << e.weight
        << std::endl;
  }
  out.close();
}

}  // namespace rime
