// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-02 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <boost/foreach.hpp>
#include <rime/impl/table.h>

namespace {

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  int num_syllables;
  int num_entries;
};

const char kTableFormat[] = "Rime::Table/0.9";

}  // namespace

namespace rime {

bool Code::operator< (const Code &other) const {
  if (size() != other.size())
    return size() < other.size();
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return at(i) < other.at(i);
  }
  return false;
}

bool Code::operator== (const Code &other) const {
  if (size() != other.size())
    return false;
  for (size_t i = 0; i < size(); ++i) {
    if (at(i) != other.at(i))
      return false;
  }
  return true;
}

bool EntryDefinition::operator< (const EntryDefinition& other) const {
  // Sort different entries sharing the same code by weight desc.
  if (weight != other.weight)
    return weight > other.weight;
  return text < other.text;
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
  std::pair<TableSyllabary*, size_t> syllabary =
      file()->find<TableSyllabary>("Syllabary");
  if (!syllabary.second) {
    EZLOGGERPRINT("Syllabary not found.");
    return false;
  }
  syllabary_ = syllabary.first;
  std::pair<TableIndex*, size_t> index =
      file()->find<TableIndex>("TableIndex");
  if (!index.second) {
    EZLOGGERPRINT("Table index not found.");
    return false;
  }
  index_ = index.first;
  return true;
}

bool Table::Save() {
  EZLOGGERPRINT("Save file: %s", file_name().c_str());

  if (!index_) {
    EZLOGGERPRINT("Error: the table has not been constructed!");
    return false;
  }

  Close();
  return ShrinkToFit();
}

bool Table::Build(const Syllabary &syllabary, const Vocabulary &vocabulary, size_t num_entries) {
  size_t num_syllables = syllabary.size();
  size_t file_size = 32 * num_syllables + 128 * num_entries;
  if (!Create(file_size)) {
    EZLOGGERPRINT("Error creating table file '%s'.", file_name().c_str());
    return false;
  }
  {
    Metadata *metadata = file()->construct<Metadata>("Metadata")();
    if (!metadata) {
      EZLOGGERPRINT("Error writing metadata into file '%s'.", file_name().c_str());
      return false;
    }
    std::strncpy(metadata->format, kTableFormat, Metadata::kFormatMaxLength);
    metadata->num_syllables = num_syllables;
    metadata->num_entries = num_entries;
  }

  VoidAllocator void_allocator(file()->get_segment_manager());
  if (!syllabary_) {
    syllabary_ = file()->construct<TableSyllabary>("Syllabary", std::nothrow)(void_allocator);
    if (!syllabary_) {
      EZLOGGERPRINT("Error creating syllabary.");
      return false;
    }
    syllabary_->reserve(num_syllables);
    BOOST_FOREACH(const std::string &syllable, syllabary) {
      String str(syllable.c_str(), void_allocator);
      syllabary_->push_back(boost::interprocess::move(str));
    }
  }
  if (!index_) {
    index_ = file()->construct<TableIndex>("TableIndex", std::nothrow)(void_allocator);
    if (!index_) {
      EZLOGGERPRINT("Error creating table index.");
      return false;
    }
    index_->resize(num_syllables);
  }
  
  for (Vocabulary::const_iterator v = vocabulary.begin(); v != vocabulary.end(); ++v) {
    const Code &code(v->first);
    const std::vector<EntryDefinition> &definitions(v->second);
    // For now only Level 1 index is supported...
    if (code.size() > 1)
      break;
    int syllable_id = code[0];
    TableIndexNode &node(index_->at(syllable_id));
    TableEntryVector *entries = NULL;
    if (node.entries) {
      // Already there?!
      entries = node.entries.get();
    }
    else {
      entries = file()->construct<TableEntryVector>(boost::interprocess::anonymous_instance)(
          void_allocator);
      if (!entries) {
        EZLOGGERPRINT("Error creating table entries; file size: %u, free memory: %u.",
                      file()->get_size(), file()->get_free_memory());
        return false;
      }
      entries->reserve(entries->size() + definitions.size());
      node.entries = entries;
    }
    for (std::vector<EntryDefinition>::const_iterator d = definitions.begin(); d != definitions.end(); ++d) {
      TableEntry *entry = file()->construct<TableEntry>(boost::interprocess::anonymous_instance)(
          d->text.c_str(),
          d->weight,
          void_allocator);
      if (!entry) {
        EZLOGGERPRINT("Error creating table entry '%s'; file size: %u, free memory: %u.",
                      d->text.c_str(), file()->get_size(), file()->get_free_memory());
        return false;
      }
      entries->push_back(boost::interprocess::move(*entry));
      file()->destroy_ptr(entry);
    }
  }

  return true;
}

const char* Table::GetSyllable(int syllable_id) {
  if (!syllabary_ ||
      syllable_id < 0 ||
      syllable_id >= syllabary_->size())
    return NULL;
  return (*syllabary_)[syllable_id].c_str();
}

const TableEntryVector* Table::GetEntries(int syllable_id) {
  if (!index_)
    return NULL;
  if (syllable_id >= index_->size())
    return NULL;
  TableEntryVector *vec = index_->at(syllable_id).entries.get();
  if (!vec)
    return NULL;
  return vec;
}

}  // namespace rime
