//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-07-02 GONG Chen <chen.sst@gmail.com>
//
#include <cfloat>
#include <cstring>
#include <algorithm>
#include <queue>
#include <vector>
#include <utility>
#include <rime/algo/syllabifier.h>
#include <rime/dict/table.h>

#define RIME_THIS_CALL(f) (this->*(f))

namespace rime {

const char kTableFormat_v1[] = "Rime::Table/1.0";
const char kTableFormat_v2[] = "Rime::Table/2.0";

const char kTableFormatPrefix[] = "Rime::Table/";
const size_t kTableFormatPrefixLen = sizeof(kTableFormatPrefix) - 1;

class TableQuery {
 public:
  TableQuery() {
    Reset();
  }
  virtual ~TableQuery() = default;

  virtual TableAccessor Access(int syllable_id,
                               double credibility = 1.0) const = 0;

  virtual bool Walk(int syllable_id, double credibility = 1.0) = 0;

  virtual TableQuery* NewCopy() const = 0;

  // down to next level
  bool Advance(int syllable_id, double credibility);
  // up one level
  bool Backdate();
  // back to root
  void Reset();

  size_t level() const { return level_; }

 protected:
  size_t level_ = 0;
  Code index_code_;
  std::vector<double> credibility_;
};

TableAccessor::TableAccessor(const Code& index_code,
                             const List<table::Entry>* entries,
                             double credibility)
    : index_code_(index_code), entries_(entries), credibility_(credibility) {
}

TableAccessor::TableAccessor(const Code& index_code,
                             const table::TailIndex* code_map,
                             double credibility)
    : index_code_(index_code), code_map_(code_map), credibility_(credibility) {
}

bool TableAccessor::exhausted() const {
  if (entries_) return cursor_ >= entries_->size;
  if (code_map_) return cursor_ >= code_map_->size;
  return true;
}

size_t TableAccessor::remaining() const {
  if (entries_) return entries_->size - cursor_;
  if (code_map_) return code_map_->size - cursor_;
  return 0;
}

const table::Entry* TableAccessor::entry() const {
  if (exhausted())
    return NULL;
  if (entries_)
    return &entries_->at[cursor_];
  else
    return &code_map_->at[cursor_].entry;
}

const table::Code* TableAccessor::extra_code() const {
  if (!code_map_ || cursor_ >= code_map_->size)
    return NULL;
  return &code_map_->at[cursor_].extra_code;
}

Code TableAccessor::code() const {
  auto extra = extra_code();
  if (!extra) {
    return index_code();
  }
  Code code(index_code());
  for (auto p = extra->begin(); p != extra->end(); ++p) {
    code.push_back(*p);
  }
  return code;
}

bool TableAccessor::Next() {
  if (exhausted())
    return false;
  ++cursor_;
  return !exhausted();
}

bool TableQuery::Advance(int syllable_id, double credibility) {
  ++level_;
  index_code_.push_back(syllable_id);
  credibility_.push_back(credibility_.back() * credibility);
  return true;
}

bool TableQuery::Backdate() {
  if (level_ == 0)
    return false;
  --level_;
  if (index_code_.size() > level_) {
    index_code_.pop_back();
    credibility_.pop_back();
  }
  return true;
}

void TableQuery::Reset() {
  level_ = 0;
  index_code_.clear();
  credibility_.clear();
  credibility_.push_back(1.0);
}

class TableQuery_v1 : public TableQuery {
 public:
  TableQuery_v1(table::Index* index) : lv1_index_(&index->v1) {
  }

  TableAccessor Access(int syllable_id, double credibility) const;
  bool Walk(int syllable_id, double credibility);

  TableQuery* NewCopy() const {
    return new TableQuery_v1(*this);
  }

 private:
  table::HeadIndex_v1* lv1_index_ = nullptr;
  table::TrunkIndex_v1* lv2_index_ = nullptr;
  table::TrunkIndex_v1* lv3_index_ = nullptr;
  table::TailIndex* lv4_index_ = nullptr;
};

inline static bool node_less(const table::TrunkIndexNode_v1& a,
                             const table::TrunkIndexNode_v1& b) {
  return a.key < b.key;
}

static table::TrunkIndexNode_v1* find_node(table::TrunkIndexNode_v1* first,
                                           table::TrunkIndexNode_v1* last,
                                           const SyllableId& key) {
  table::TrunkIndexNode_v1 target;
  target.key = key;
  auto it = std::lower_bound(first, last, target, node_less);
  return it == last || key < it->key ? last : it;
}

bool TableQuery_v1::Walk(int syllable_id, double credibility) {
  if (level_ == 0) {
    if (!lv1_index_ ||
        syllable_id < 0 ||
        syllable_id >= static_cast<int>(lv1_index_->size))
      return false;
    auto node = &lv1_index_->at[syllable_id];
    if (!node->next_level)
      return false;
    lv2_index_ = &node->next_level->trunk_v1;
  }
  else if (level_ == 1) {
    if (!lv2_index_)
      return false;
    auto node = find_node(lv2_index_->begin(), lv2_index_->end(), syllable_id);
    if (node == lv2_index_->end())
      return false;
    if (!node->next_level)
      return false;
    lv3_index_ = &node->next_level->trunk_v1;
  }
  else if (level_ == 2) {
    if (!lv3_index_)
      return false;
    auto node = find_node(lv3_index_->begin(), lv3_index_->end(), syllable_id);
    if (node == lv3_index_->end())
      return false;
    if (!node->next_level)
      return false;
    lv4_index_ = &node->next_level->tail;
  }
  else {
    return false;
  }
  Advance(syllable_id, credibility);
  return true;
}

inline static Code add_syllable(Code code, int syllable_id) {
  code.push_back(syllable_id);
  return code;
}

TableAccessor TableQuery_v1::Access(int syllable_id,
                                    double credibility) const {
  credibility *= credibility_.back();
  if (level_ == 0) {
    if (!lv1_index_ ||
        syllable_id < 0 ||
        syllable_id >= static_cast<int>(lv1_index_->size))
      return TableAccessor();
    auto node = &lv1_index_->at[syllable_id];
    return TableAccessor(add_syllable(index_code_, syllable_id),
                         &node->entries, credibility);
  }
  else if (level_ == 1 || level_ == 2) {
    auto index = (level_ == 1) ? lv2_index_ : lv3_index_;
    if (!index)
      return TableAccessor();
    auto node = find_node(index->begin(), index->end(), syllable_id);
    if (node == index->end())
      return TableAccessor();
    return TableAccessor(add_syllable(index_code_, syllable_id),
                         &node->entries, credibility);
  }
  else if (level_ == 3) {
    if (!lv4_index_)
      return TableAccessor();
    return TableAccessor(index_code_, lv4_index_, credibility);
  }
  return TableAccessor();
}

TableQuery* Table::NewQuery_v1() {
  return new TableQuery_v1(index_);
}

std::string Table::GetString_v1(const table::StringType& x) {
 return x.str.c_str();
}
bool Table::AddString_v1(const std::string& src, table::StringType* dest,
                         double /*weight*/) {
  return CopyString(src, &dest->str);
}
table::Index* Table::BuildIndex_v1(const Vocabulary& vocabulary,
                                   size_t num_syllables) {
  return reinterpret_cast<table::Index*>(
      BuildHeadIndex(vocabulary, num_syllables));
}

TableQuery* Table::NewQuery_v2() {
  // TODO:
  return nullptr;
}

std::string Table::GetString_v2(const table::StringType& x) {
  return string_table_->GetString(x.str_id);
}

bool Table::AddString_v2(const std::string& src, table::StringType* dest,
                         double weight) {
  string_table_builder_->Add(src, weight, &dest->str_id);
  return true;
}

table::Index* Table::BuildIndex_v2(const Vocabulary& vocabulary,
                                   size_t num_syllables) {
  // TODO:
  return nullptr;
}

bool Table::OnBuildStart_v2() {
  string_table_builder_.reset(new StringTableBuilder);
  return true;
}

bool Table::OnBuildFinish_v2() {
  string_table_builder_->Build();
  // saving string table image
  size_t image_size = string_table_builder_->BinarySize();
  char* image = Allocate<char>(image_size);
  if (!image) {
    LOG(ERROR) << "Error creating string table image.";
    return false;
  }
  string_table_builder_->Dump(image, image_size);
  metadata_->string_table = image;
  metadata_->string_table_size = image_size;
  return true;
}

bool Table::OnLoad_v2() {
  string_table_.reset(new StringTable(metadata_->string_table.get(),
                                      metadata_->string_table_size));
  return true;
}

void Table::SelectTableFormat(double format_version) {
  if (format_version > 2.0 - DBL_EPSILON) {
    format_.format_name = kTableFormat_v2;
    format_.NewQuery = &Table::NewQuery_v2;
    format_.GetString = &Table::GetString_v2;
    format_.AddString = &Table::AddString_v2;
    format_.BuildIndex = &Table::BuildIndex_v2;
    format_.OnBuildStart = &Table::OnBuildStart_v2;
    format_.OnBuildFinish = &Table::OnBuildFinish_v2;
    format_.OnLoad = &Table::OnLoad_v2;
  }
  else {
    format_.format_name = kTableFormat_v1;
    format_.NewQuery = &Table::NewQuery_v1;
    format_.GetString = &Table::GetString_v1;
    format_.AddString = &Table::AddString_v1;
    format_.BuildIndex = &Table::BuildIndex_v1;
    format_.OnBuildStart = nullptr;
    format_.OnBuildFinish = nullptr;
    format_.OnLoad = nullptr;
  }
}

Table::Table(const std::string& file_name) : MappedFile(file_name) {
}

Table::~Table() {
}

bool Table::Load() {
  LOG(INFO) << "loading table file: " << file_name();

  if (IsOpen())
    Close();

  if (!OpenReadOnly()) {
    LOG(ERROR) << "Error opening table file '" << file_name() << "'.";
    return false;
  }

  metadata_ = Find<table::Metadata>(0);
  if (!metadata_) {
    LOG(ERROR) << "metadata not found.";
    Close();
    return false;
  }
  if (strncmp(metadata_->format, kTableFormatPrefix, kTableFormatPrefixLen)) {
    LOG(ERROR) << "invalid metadata.";
    Close();
    return false;
  }
  double format_version = atof(&metadata_->format[kTableFormatPrefixLen]);
  SelectTableFormat(format_version);
  format_.format_name = metadata_->format;

  syllabary_ = metadata_->syllabary.get();
  if (!syllabary_) {
    LOG(ERROR) << "syllabary not found.";
    Close();
    return false;
  }
  index_ = metadata_->index.get();
  if (!index_) {
    LOG(ERROR) << "table index not found.";
    Close();
    return false;
  }

  if (format_.OnLoad && !RIME_THIS_CALL(format_.OnLoad)()) {
    return false;
  }
  return true;
}

bool Table::Save() {
  LOG(INFO) << "saving table file: " << file_name();

  if (!index_) {
    LOG(ERROR) << "the table has not been constructed!";
    return false;
  }

  return ShrinkToFit();
}

uint32_t Table::dict_file_checksum() const {
  return metadata_ ? metadata_->dict_file_checksum : 0;
}

bool Table::Build(const Syllabary& syllabary, const Vocabulary& vocabulary,
                  size_t num_entries, uint32_t dict_file_checksum) {
  SelectTableFormat(1.0);

  size_t num_syllables = syllabary.size();
  size_t estimated_file_size = 4096 + 32 * num_syllables + 64 * num_entries;
  LOG(INFO) << "building table.";
  LOG(INFO) << "num syllables: " << num_syllables;
  LOG(INFO) << "num entries: " << num_entries;
  LOG(INFO) << "estimated file size: " << estimated_file_size;
  if (!Create(estimated_file_size)) {
    LOG(ERROR) << "Error creating table file '" << file_name() << "'.";
    return false;
  }

  LOG(INFO) << "creating metadata.";
  metadata_ = Allocate<table::Metadata>();
  if (!metadata_) {
    LOG(ERROR) << "Error creating metadata in file '" << file_name() << "'.";
    return false;
  }
  metadata_->dict_file_checksum = dict_file_checksum;
  metadata_->num_syllables = num_syllables;
  metadata_->num_entries = num_entries;

  if (format_.OnBuildStart && !RIME_THIS_CALL(format_.OnBuildStart)()) {
    return false;
  }

  LOG(INFO) << "creating syllabary.";
  syllabary_ = CreateArray<table::StringType>(num_syllables);
  if (!syllabary_) {
    LOG(ERROR) << "Error creating syllabary.";
    return false;
  }
  else {
    size_t i = 0;
    for (const std::string& syllable : syllabary) {
      RIME_THIS_CALL(format_.AddString)(syllable, &syllabary_->at[i++], 0.0);
    }
  }
  metadata_->syllabary = syllabary_;

  LOG(INFO) << "creating table index.";
  index_ = RIME_THIS_CALL(format_.BuildIndex)(vocabulary, num_syllables);
  if (!index_) {
    LOG(ERROR) << "Error creating table index.";
    return false;
  }
  metadata_->index = index_;

  if (format_.OnBuildFinish && !RIME_THIS_CALL(format_.OnBuildFinish)()) {
    return false;
  }

  // at last, complete the metadata
  std::strncpy(metadata_->format, format_.format_name,
               table::Metadata::kFormatMaxLength);
  return true;
}

table::HeadIndex_v1* Table::BuildHeadIndex(const Vocabulary& vocabulary,
                                           size_t num_syllables) {
  auto index = CreateArray<table::HeadIndexNode_v1>(num_syllables);
  if (!index) {
    return NULL;
  }
  for (const auto& v : vocabulary) {
    int syllable_id = v.first;
    auto& node(index->at[syllable_id]);
    const auto& entries(v.second.entries);
    if (!BuildEntryList(entries, &node.entries)) {
        return NULL;
    }
    if (v.second.next_level) {
      Code code;
      code.push_back(syllable_id);
      auto next_level_index = BuildTrunkIndex(code, *v.second.next_level);
      if (!next_level_index) {
        return NULL;
      }
      node.next_level = reinterpret_cast<table::PhraseIndex*>(next_level_index);
    }
  }
  return index;
}

table::TrunkIndex_v1* Table::BuildTrunkIndex(const Code& prefix,
                                          const Vocabulary& vocabulary) {
  auto index = CreateArray<table::TrunkIndexNode_v1>(vocabulary.size());
  if (!index) {
    return NULL;
  }
  size_t count = 0;
  for (const auto& v : vocabulary) {
    int syllable_id = v.first;
    auto& node(index->at[count++]);
    node.key = syllable_id;
    const auto& entries(v.second.entries);
    if (!BuildEntryList(entries, &node.entries)) {
        return NULL;
    }
    if (v.second.next_level) {
      Code code(prefix);
      code.push_back(syllable_id);
      if (code.size() < Code::kIndexCodeMaxLength) {
        auto next_level_index = BuildTrunkIndex(code, *v.second.next_level);
        if (!next_level_index) {
          return NULL;
        }
        node.next_level =
            reinterpret_cast<table::PhraseIndex*>(next_level_index);
      }
      else {
        auto tail_index = BuildTailIndex(code, *v.second.next_level);
        if (!tail_index) {
          return NULL;
        }
        node.next_level = reinterpret_cast<table::PhraseIndex*>(tail_index);
      }
    }
  }
  return index;
}

table::TailIndex* Table::BuildTailIndex(const Code& prefix,
                                        const Vocabulary& vocabulary) {
  if (vocabulary.find(-1) == vocabulary.end()) {
    return NULL;
  }
  const auto& page(vocabulary.find(-1)->second);
  DLOG(INFO) << "page size: " << page.entries.size();
  auto index = CreateArray<table::LongEntry>(page.entries.size());
  if (!index) {
    return NULL;
  }
  size_t count = 0;
  for (const auto& src : page.entries) {
    DLOG(INFO) << "count: " << count;
    DLOG(INFO) << "entry: " << src->text;
    auto& dest(index->at[count++]);
    size_t extra_code_length = src->code.size() - Code::kIndexCodeMaxLength;
    DLOG(INFO) << "extra code length: " << extra_code_length;
    dest.extra_code.size = extra_code_length;
    dest.extra_code.at = Allocate<table::SyllableId>(extra_code_length);
    if (!dest.extra_code.at) {
      LOG(ERROR) << "Error creating code sequence; file size: " << file_size();
      return NULL;
    }
    std::copy(src->code.begin() + Code::kIndexCodeMaxLength,
              src->code.end(),
              dest.extra_code.begin());
    BuildEntry(*src, &dest.entry);
  }
  return index;
}

bool Table::BuildEntryList(const DictEntryList& src,
                               List<table::Entry>* dest) {
  if (!dest)
    return false;
  dest->size = src.size();
  dest->at = Allocate<table::Entry>(src.size());
  if (!dest->at) {
    LOG(ERROR) << "Error creating table entries; file size: " << file_size();
    return false;
  }
  size_t i = 0;
  for (auto d = src.begin(); d != src.end(); ++d, ++i) {
    if (!BuildEntry(**d, &dest->at[i]))
      return false;
  }
  return true;
}

bool Table::BuildEntry(const DictEntry& dict_entry, table::Entry* entry) {
  if (!entry)
    return false;
  if (!RIME_THIS_CALL(format_.AddString)(dict_entry.text, &entry->text,
                                         dict_entry.weight)) {
    LOG(ERROR) << "Error creating table entry '" << dict_entry.text
               << "'; file size: " << file_size();
    return false;
  }
  entry->weight = static_cast<table::Weight>(dict_entry.weight);
  return true;
}

bool Table::GetSyllabary(Syllabary* result) {
  if (!result || !syllabary_)
    return false;
  for (size_t i = 0; i < syllabary_->size; ++i) {
    result->insert(GetSyllableById((int)i));
  }
  return true;
}
std::string Table::GetSyllableById(int syllable_id) {
  if (!syllabary_ ||
      syllable_id < 0 ||
      syllable_id >= static_cast<int>(syllabary_->size))
    return std::string();
  return RIME_THIS_CALL(format_.GetString)(syllabary_->at[syllable_id]);
}

TableAccessor Table::QueryWords(int syllable_id) {
  unique_ptr<TableQuery> query(RIME_THIS_CALL(format_.NewQuery)());
  return query->Access(syllable_id);
}

TableAccessor Table::QueryPhrases(const Code& code) {
  if (code.empty())
    return TableAccessor();
  unique_ptr<TableQuery> query(RIME_THIS_CALL(format_.NewQuery)());
  for (size_t i = 0; i < Code::kIndexCodeMaxLength; ++i) {
    if (code.size() == i + 1)
      return query->Access(code[i]);
    if (!query->Walk(code[i]))
      return TableAccessor();
  }
  return query->Access(-1);
}

bool Table::Query(const SyllableGraph& syll_graph, size_t start_pos,
                  TableQueryResult* result) {
  if (!result ||
      !index_ ||
      start_pos >= syll_graph.interpreted_length)
    return false;
  result->clear();
  std::queue<std::pair<size_t, TableQuery*>> q;
  TableQuery* initial_state = RIME_THIS_CALL(format_.NewQuery)();
  q.push({start_pos, initial_state});
  while (!q.empty()) {
    int current_pos = q.front().first;
    unique_ptr<TableQuery> query(q.front().second);
    q.pop();
    auto index = syll_graph.indices.find(current_pos);
    if (index == syll_graph.indices.end()) {
      continue;
    }
    if (query->level() == Code::kIndexCodeMaxLength) {
      TableAccessor accessor(query->Access(-1));
      if (!accessor.exhausted()) {
        (*result)[current_pos].push_back(accessor);
      }
      continue;
    }
    for (const auto& spellings : index->second) {
      SyllableId syll_id = spellings.first;
      TableAccessor accessor(query->Access(syll_id));
      for (auto props : spellings.second) {
        size_t end_pos = props->end_pos;
        if (!accessor.exhausted()) {
          (*result)[end_pos].push_back(accessor);
        }
        if (end_pos < syll_graph.interpreted_length &&
            query->Walk(syll_id, props->credibility)) {
          q.push({end_pos, query->NewCopy()});
          query->Backdate();
        }
      }
    }
  }
  return !result->empty();
}

std::string Table::GetEntryText(const table::Entry& entry) {
  return RIME_THIS_CALL(format_.GetString)(entry.text);
}

}  // namespace rime
