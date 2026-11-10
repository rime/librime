//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
// 2014-07-06 GONG Chen <chen.sst@gmail.com> redesigned binary file format.
//
#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string_view>
#include <boost/algorithm/string.hpp>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/ticket.h>
#include <rime/dict/db_pool_impl.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/reverse_lookup_dictionary.h>

namespace rime {

const char kReverseFormat[] = "Rime::Reverse/3.1";
const double kReverseFormatCompatible = 3.0;

const char kReverseFormatPrefix[] = "Rime::Reverse/";
const size_t kReverseFormatPrefixLen = sizeof(kReverseFormatPrefix) - 1;

static const char* kStemKeySuffix = "\x1fstem";

ReverseDb::ReverseDb(const path& file_path) : MappedFile(file_path) {}

bool ReverseDb::Load() {
  LOG(INFO) << "loading reversedb: " << file_path();

  if (IsOpen())
    Close();

  if (!OpenReadOnly()) {
    LOG(ERROR) << "Error opening reversedb '" << file_path() << "'.";
    return false;
  }

  metadata_ = Find<reverse::Metadata>(0);
  if (!metadata_) {
    LOG(ERROR) << "metadata not found.";
    Close();
    return false;
  }
  if (strncmp(metadata_->format, kReverseFormatPrefix,
              kReverseFormatPrefixLen)) {
    LOG(ERROR) << "invalid metadata.";
    Close();
    return false;
  }
  double format = std::atof(&metadata_->format[kReverseFormatPrefixLen]);
  if (format - kReverseFormatCompatible < 0.0 - DBL_EPSILON ||
      format - kReverseFormatCompatible > 1.0 + DBL_EPSILON) {
    LOG(ERROR) << "incompatible reversedb format.";
    Close();
    return false;
  }

  key_trie_.reset(
      new StringTable(metadata_->key_trie.get(), metadata_->key_trie_size));
  value_trie_.reset(
      new StringTable(metadata_->value_trie.get(), metadata_->value_trie_size));

  return true;
}

bool ReverseDb::Lookup(const string& text, string* result) {
  if (!key_trie_ || !value_trie_ || !metadata_->index.size) {
    return false;
  }
  StringId key_id = key_trie_->Lookup(text);
  if (key_id == kInvalidStringId) {
    return false;
  }
  StringId value_id = metadata_->index.at[key_id];
  *result = value_trie_->GetString(value_id);
  return !result->empty();
}

bool ReverseDb::Build(DictSettings* settings,
                      const Syllabary& syllabary,
                      const Vocabulary& vocabulary,
                      const ReverseLookupTable& stems,
                      uint32_t dict_file_checksum) {
  // Compatibility path for callers that still hold a full Vocabulary.
  ReverseEntryList entries;
  for (SyllableId syllable_id = 0;
       syllable_id < static_cast<SyllableId>(syllabary.size()); ++syllable_id) {
    auto it = vocabulary.find(syllable_id);
    if (it == vocabulary.end())
      continue;
    for (const auto& e : it->second.entries) {
      if (e.text.size() > (std::numeric_limits<uint32_t>::max)()) {
        LOG(ERROR) << "dictionary entry text is too long: '" << e.text << "'";
        return false;
      }
      entries.entries.push_back({entries.text.size(),
                                 static_cast<uint32_t>(e.text.size()),
                                 syllable_id, e.weight});
      entries.text.append(e.text);
    }
  }
  return Build(settings, syllabary, entries, stems, dict_file_checksum);
}

bool ReverseDb::Build(DictSettings* settings,
                      const Syllabary& syllabary,
                      ReverseEntryList& entries,
                      const ReverseLookupTable& stems,
                      uint32_t dict_file_checksum) {
  LOG(INFO) << "building reversedb...";
  vector<std::string_view> syllables;
  syllables.reserve(syllabary.size());
  for (const auto& syllable : syllabary)
    syllables.push_back(syllable);

  std::sort(entries.entries.begin(), entries.entries.end(),
            [&](const CompactDictEntry& a, const CompactDictEntry& b) {
              std::string_view ta = entries.text_of(a);
              std::string_view tb = entries.text_of(b);
              if (ta != tb)
                return ta < tb;
              return syllables[a.syllable_id] < syllables[b.syllable_id];
            });
  size_t num_words = 0;
  for (size_t j = 0; j < entries.entries.size(); ++j) {
    if (j == 0 || entries.text_of(entries.entries[j]) !=
                      entries.text_of(entries.entries[j - 1]))
      ++num_words;
  }

  const size_t entry_count = num_words + stems.size();
  vector<StringId> key_ids(entry_count);
  vector<StringId> value_ids(entry_count);

  // Build the key trie first and serialize it, then release its working data
  // before building the value trie. The two tries are never needed in memory
  // at the same time.
  StringTableBuilder key_trie_builder;
  {
    size_t i = 0;
    for (size_t j = 0; j < entries.entries.size();) {
      const std::string_view text = entries.text_of(entries.entries[j]);
      key_trie_builder.AddView(text, 0.0, &key_ids[i++]);
      while (j < entries.entries.size() &&
             entries.text_of(entries.entries[j]) == text)
        ++j;
    }
    for (const auto& v : stems) {
      string key(v.first + kStemKeySuffix);
      key_trie_builder.Add(key, 0.0, &key_ids[i++]);
    }
  }
  key_trie_builder.Build();
  vector<char> key_trie_image(key_trie_builder.BinarySize());
  key_trie_builder.Dump(key_trie_image.data(), key_trie_image.size());
  key_trie_builder.Clear();

  StringTableBuilder value_trie_builder;
  {
    string value;
    size_t i = 0;
    for (size_t j = 0; j < entries.entries.size();) {
      const std::string_view text = entries.text_of(entries.entries[j]);
      value.clear();
      SyllableId previous = 0;
      bool first = true;
      for (; j < entries.entries.size() &&
             entries.text_of(entries.entries[j]) == text;
           ++j) {
        const SyllableId syllable_id = entries.entries[j].syllable_id;
        if (!first && syllable_id == previous)
          continue;
        if (!first)
          value.push_back(' ');
        const std::string_view syllable = syllables[syllable_id];
        value.append(syllable.data(), syllable.size());
        previous = syllable_id;
        first = false;
      }
      value_trie_builder.Add(value, 0.0, &value_ids[i++]);
    }
    for (const auto& v : stems) {
      string value(boost::algorithm::join(v.second, " "));
      value_trie_builder.Add(value, 0.0, &value_ids[i++]);
    }
  }
  value_trie_builder.Build();
  vector<char> value_trie_image(value_trie_builder.BinarySize());
  value_trie_builder.Dump(value_trie_image.data(), value_trie_image.size());
  value_trie_builder.Clear();

  // dict settings required by UniTE
  string dict_settings;
  if (settings && settings->use_rule_based_encoder()) {
    std::ostringstream yaml;
    settings->SaveToStream(yaml);
    dict_settings = yaml.str();
  }

  // creating reversedb file
  const size_t kReservedSize = 1024;
  const size_t key_trie_image_size = key_trie_image.size();
  const size_t value_trie_image_size = value_trie_image.size();
  size_t estimated_data_size = kReservedSize + dict_settings.length() +
                               entry_count * sizeof(StringId) +
                               key_trie_image_size + value_trie_image_size;
  if (!Create(estimated_data_size)) {
    LOG(ERROR) << "Error creating prism file '" << file_path() << "'.";
    return false;
  }

  // create metadata
  metadata_ = Allocate<reverse::Metadata>();
  if (!metadata_) {
    LOG(ERROR) << "Error creating metadata in file '" << file_path() << "'.";
    return false;
  }
  metadata_->dict_file_checksum = dict_file_checksum;
  if (!dict_settings.empty()) {
    if (!CopyString(dict_settings, &metadata_->dict_settings)) {
      LOG(ERROR) << "Error saving dict settings.";
      return false;
    }
  }

  auto index = Allocate<StringId>(entry_count);
  if (!index) {
    return false;
  }
  for (size_t k = 0; k < entry_count; ++k) {
    index[key_ids[k]] = value_ids[k];
  }
  metadata_->index.size = entry_count;
  metadata_->index.at = index;

  // save key trie image
  char* key_trie_ptr = Allocate<char>(key_trie_image_size);
  if (!key_trie_ptr) {
    LOG(ERROR) << "Error creating key trie image.";
    return false;
  }
  std::memcpy(key_trie_ptr, key_trie_image.data(), key_trie_image_size);
  metadata_->key_trie = key_trie_ptr;
  metadata_->key_trie_size = key_trie_image_size;

  // save value trie image
  char* value_trie_ptr = Allocate<char>(value_trie_image_size);
  if (!value_trie_ptr) {
    LOG(ERROR) << "Error creating value trie image.";
    return false;
  }
  std::memcpy(value_trie_ptr, value_trie_image.data(), value_trie_image_size);
  metadata_->value_trie = value_trie_ptr;
  metadata_->value_trie_size = value_trie_image_size;

  // at last, complete the metadata
  std::strncpy(metadata_->format, kReverseFormat,
               reverse::Metadata::kFormatMaxLength);
  return true;
}

bool ReverseDb::Save() {
  LOG(INFO) << "saving reverse file: " << file_path();
  return ShrinkToFit();
}

uint32_t ReverseDb::dict_file_checksum() const {
  return metadata_ ? metadata_->dict_file_checksum : 0;
}

ReverseLookupDictionary::ReverseLookupDictionary(an<ReverseDb> db) : db_(db) {}

bool ReverseLookupDictionary::Load() {
  return db_ && (db_->IsOpen() || db_->Load());
}

bool ReverseLookupDictionary::ReverseLookup(const string& text,
                                            string* result) {
  return db_->Lookup(text, result);
}

bool ReverseLookupDictionary::LookupStems(const string& text, string* result) {
  return db_->Lookup(text + kStemKeySuffix, result);
}

an<DictSettings> ReverseLookupDictionary::GetDictSettings() {
  an<DictSettings> settings;
  reverse::Metadata* metadata = db_->metadata();
  if (metadata && !metadata->dict_settings.empty()) {
    string yaml(metadata->dict_settings.c_str());
    std::istringstream iss(yaml);
    settings = New<DictSettings>();
    if (!settings->LoadFromStream(iss)) {
      settings.reset();
    }
  }
  return settings;
}

static const ResourceType kReverseDbResourceType = {"reverse_db", "",
                                                    ".reverse.bin"};

ReverseLookupDictionaryComponent::ReverseLookupDictionaryComponent()
    : DbPool(the<ResourceResolver>(
          Service::instance().CreateDeployedResourceResolver(
              kReverseDbResourceType))) {}

ReverseLookupDictionary* ReverseLookupDictionaryComponent::Create(
    const string& dict_name) {
  auto db = GetDb(dict_name);
  return new ReverseLookupDictionary(db);
};

ReverseLookupDictionary* ReverseLookupDictionaryComponent::Create(
    const Ticket& ticket) {
  if (!ticket.schema)
    return NULL;
  Config* config = ticket.schema->config();
  string dict_name;
  if (!config->GetString(ticket.name_space + "/dictionary", &dict_name)) {
    // missing!
    return NULL;
  }
  return Create(dict_name);
}

}  // namespace rime
