// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-02 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <algorithm>
#include <vector>
#include <utility>
#include <boost/foreach.hpp>
#include <rime/impl/table.h>

namespace rime {

const char kTableFormat[] = "Rime::Table/0.9";

TableVisitor::TableVisitor()
    : entries_(NULL), code_map_(NULL), cursor_(0) {
}

TableVisitor::TableVisitor(const List<table::Entry> *entries)
    : entries_(entries), code_map_(NULL), cursor_(0) {
}

TableVisitor::TableVisitor(const table::TailIndex *code_map)
    : entries_(NULL), code_map_(code_map), cursor_(0) {
}

bool TableVisitor::exhausted() const {
  if (entries_) return cursor_ >= entries_->size;
  if (code_map_) return cursor_ >= code_map_->size;
  return true;
}

size_t TableVisitor::remaining() const {
  if (entries_) return entries_->size - cursor_;
  if (code_map_) return code_map_->size - cursor_;
  return 0;
}

const table::Entry* TableVisitor::entry() const {
  if (exhausted())
    return NULL;
  if (entries_)
    return &entries_->at[cursor_];
  else
    return &code_map_->at[cursor_].entry;
}

const table::Code* TableVisitor::extra_code() const {
  if (!code_map_ || cursor_ >= code_map_->size)
    return NULL;
  return &code_map_->at[cursor_].extra_code;
}

bool TableVisitor::Next() {
  if (exhausted())
    return false;
  ++cursor_;
  return !exhausted();
}

bool Table::Load() {
  EZLOGGERPRINT("Load file: %s", file_name().c_str());

  if (IsOpen())
    Close();
  
  if (!OpenReadOnly()) {
    EZLOGGERPRINT("Error opening table file '%s'.",
                  file_name().c_str());
    return false;
  }

  metadata_ = Find<table::Metadata>(0);
  if (!metadata_) {
    EZLOGGERPRINT("Metadata not found.");
    return false;
  }
  syllabary_ = metadata_->syllabary.get();
  if (!syllabary_) {
    EZLOGGERPRINT("Syllabary not found.");
    return false;
  }
  index_ = metadata_->index.get();
  if (!index_) {
    EZLOGGERPRINT("Table index not found.");
    return false;
  }
  return true;
}

bool Table::Save() {
  EZLOGGERPRINT("Save file: %s", file_name().c_str());

  if (!index_) {
    EZLOGGERPRINT("Error: the table has not been constructed!");
    return false;
  }

  return ShrinkToFit();
}

bool Table::Build(const Syllabary &syllabary, const Vocabulary &vocabulary, size_t num_entries) {
  size_t num_syllables = syllabary.size();
  size_t estimated_file_size = 32 * num_syllables + 128 * num_entries;
  EZLOGGERVAR(num_syllables);
  EZLOGGERVAR(num_entries);
  EZLOGGERVAR(estimated_file_size);
  if (!Create(estimated_file_size)) {
    EZLOGGERPRINT("Error creating table file '%s'.", file_name().c_str());
    return false;
  }

  EZLOGGERPRINT("Creating metadata.");
  metadata_ = Allocate<table::Metadata>();
  if (!metadata_) {
    EZLOGGERPRINT("Error creating metadata in file '%s'.", file_name().c_str());
    return false;
  }
  std::strncpy(metadata_->format, kTableFormat, table::Metadata::kFormatMaxLength);
  metadata_->num_syllables = num_syllables;
  metadata_->num_entries = num_entries;

  EZLOGGERPRINT("Creating syllabary.");
  syllabary_ = CreateArray<String>(num_syllables);
  if (!syllabary_) {
    EZLOGGERPRINT("Error creating syllabary.");
    return false;
  }
  else {
    size_t i = 0;
    BOOST_FOREACH(const std::string &syllable, syllabary) {
      CopyString(syllable, &syllabary_->at[i++]);
    }
  }
  metadata_->syllabary = syllabary_;

  EZLOGGERPRINT("Creating table index.");
  index_ = BuildHeadIndex(vocabulary, num_syllables);
  if (!index_) {
    EZLOGGERPRINT("Error creating table index.");
    return false;
  }
  metadata_->index = index_;
  
  return true;
}

table::HeadIndex* Table::BuildHeadIndex(const Vocabulary &vocabulary, size_t num_syllables) {
  table::HeadIndex *index = CreateArray<table::HeadIndexNode>(num_syllables);
  if (!index) {
    return NULL;
  }
  BOOST_FOREACH(const Vocabulary::value_type &v, vocabulary) {
    int syllable_id = v.first;
    EZDBGONLYLOGGERVAR(syllable_id);
    table::HeadIndexNode &node(index->at[syllable_id]);
    const DictEntryList &entries(v.second.entries);
    if (!BuildEntryList(entries, &node.entries)) {
        return NULL;
    }
    if (v.second.next_level) {
      Code code;
      code.push_back(syllable_id);
      table::TrunkIndex *next_level_index = BuildTrunkIndex(code, *v.second.next_level);
      if (!next_level_index) {
        return NULL;
      }
      node.next_level = reinterpret_cast<char*>(next_level_index);
    }
  }
  return index;
}

table::TrunkIndex* Table::BuildTrunkIndex(const Code &prefix, const Vocabulary &vocabulary) {
  table::TrunkIndex *index = CreateArray<table::TrunkIndexNode>(vocabulary.size());
  if (!index) {
    return NULL;
  }
  size_t count = 0;
  BOOST_FOREACH(const Vocabulary::value_type &v, vocabulary) {
    int syllable_id = v.first;
    EZDBGONLYLOGGERVAR(syllable_id);
    table::TrunkIndexNode &node(index->at[count++]);
    node.key = syllable_id;
    const DictEntryList &entries(v.second.entries);
    if (!BuildEntryList(entries, &node.entries)) {
        return NULL;
    }
    if (v.second.next_level) {
      Code code(prefix);
      code.push_back(syllable_id);
      if (code.size() < Code::kIndexCodeMaxLength) {
        table::TrunkIndex *next_level_index = BuildTrunkIndex(code, *v.second.next_level);
        if (!next_level_index) {
          return NULL;
        }
        node.next_level = reinterpret_cast<char*>(next_level_index);
      }
      else {
        table::TailIndex *tail_index = BuildTailIndex(code, *v.second.next_level);
        if (!tail_index) {
          return NULL;
        }
        node.next_level = reinterpret_cast<char*>(tail_index);
      }
    }
  }
  return index;
}

table::TailIndex* Table::BuildTailIndex(const Code &prefix, const Vocabulary &vocabulary) {
  if (vocabulary.empty()) {
    return NULL;
  }
  table::TailIndex *index = CreateArray<table::TailIndexNode>(vocabulary.size());
  if (!index) {
    return NULL;
  }
  size_t count = 0;
  const VocabularyPage &page(vocabulary.begin()->second);
  BOOST_FOREACH(const DictEntryList::value_type &src, page.entries) {
    EZDBGONLYLOGGERVAR(count);
    EZDBGONLYLOGGERVAR(src.text);
    table::TailIndexNode &dest(index->at[count++]);
    size_t extra_code_length = src.code.size() - Code::kIndexCodeMaxLength;
    EZDBGONLYLOGGERVAR(extra_code_length);
    dest.extra_code.size = extra_code_length;
    dest.extra_code.at = Allocate<table::SyllableId>(extra_code_length);
    if (!dest.extra_code.at) {
      EZLOGGERPRINT("Error creating code sequence; file size: %u.", file_size());
      return false;
    }
    std::copy(src.code.begin() + Code::kIndexCodeMaxLength,
              src.code.end(),
              dest.extra_code.at.get());
    BuildEntry(src, &dest.entry);
  }
  return index;
}

bool Table::BuildEntryList(const DictEntryList &src, List<table::Entry> *dest) {
  if (!dest)
    return false;
  dest->size = src.size();
  dest->at = Allocate<table::Entry>(src.size());
  if (!dest->at) {
    EZLOGGERPRINT("Error creating table entries; file size: %u.", file_size());
    return false;
  }
  size_t i = 0;
  for (std::vector<DictEntry>::const_iterator d = src.begin(); d != src.end(); ++d, ++i) {
    if (!BuildEntry(*d, &dest->at[i]))
      return false;
  }
  return true;
}

bool Table::BuildEntry(const DictEntry &dict_entry, table::Entry *entry) {
  if (!entry)
    return false;
  if (!CopyString(dict_entry.text, &entry->text)) {
    EZLOGGERPRINT("Error creating table entry '%s'; file size: %u.",
                  dict_entry.text.c_str(), file_size());
    return false;
  }
  entry->weight = static_cast<float>(dict_entry.weight);
  return true;
}

const char* Table::GetSyllableById(int syllable_id) {
  if (!syllabary_ ||
      syllable_id < 0 ||
      syllable_id >= syllabary_->size)
    return NULL;
  return syllabary_->at[syllable_id].c_str();
}

const TableVisitor Table::QueryWords(int syllable_id) {
  if (!index_ || syllable_id < 0 || syllable_id >= index_->size)
    return TableVisitor();
  List<table::Entry> &entries(index_->at[syllable_id].entries);
  return TableVisitor(&entries);
}

inline static bool node_less(const table::TrunkIndexNode &a,
                             const table::TrunkIndexNode &b) {
  return a.key < b.key;
}

static table::TrunkIndexNode* find_node(table::TrunkIndexNode* first,
                                        table::TrunkIndexNode* last,
                                        const table::SyllableId& key) {
  table::TrunkIndexNode target;
  target.key = key;
  table::TrunkIndexNode* it = std::lower_bound(first, last, target, node_less);
  return it == last || key < it->key ? last : it;
}

const TableVisitor Table::QueryPhrases(const Code &code) {
  if (!index_ || code.empty())
    return TableVisitor();
  table::HeadIndexNode *lv1_node = &index_->at[code[0]];
  if (code.size() == 1) {
    return TableVisitor(&lv1_node->entries);
  }
  if (!lv1_node->next_level) {
    return TableVisitor();
  }
  table::TrunkIndex *lv2_index = reinterpret_cast<table::TrunkIndex*>(
      lv1_node->next_level.get());
  table::TrunkIndexNode *lv2_node =
      find_node(lv2_index->begin(), lv2_index->end(), code[1]);
  if (lv2_node == lv2_index->end()) {
    return TableVisitor();
  }
  if (code.size() == 2) {
    return TableVisitor(&lv2_node->entries);
  }
  if (!lv2_node->next_level) {
    return TableVisitor();
  }
  table::TrunkIndex *lv3_index = reinterpret_cast<table::TrunkIndex*>(
      lv2_node->next_level.get());
  table::TrunkIndexNode *lv3_node =
      find_node(lv2_index->begin(), lv2_index->end(), code[2]);
  if (lv3_node == lv3_index->end()) {
    return TableVisitor();
  }
  if (code.size() == 3) {
    return TableVisitor(&lv3_node->entries);
  }
  if (!lv3_node->next_level) {
    return TableVisitor();
  }
  table::TailIndex *lv4_index = reinterpret_cast<table::TailIndex*>(
      lv3_node->next_level.get());
  return TableVisitor(lv4_index);
}  

}  // namespace rime
