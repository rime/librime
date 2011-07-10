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

bool Table::Load() {
  EZLOGGERPRINT("Load file: %s", file_name().c_str());

  if (IsOpen())
    Close();
  
  if (!OpenReadOnly()) {
    EZLOGGERPRINT("Error opening table file '%s'.",
                  file_name().c_str());
    return false;
  }
  std::pair<table::Syllabary*, size_t> syllabary =
      file()->find<table::Syllabary>("Syllabary");
  if (!syllabary.second) {
    EZLOGGERPRINT("Syllabary not found.");
    return false;
  }
  syllabary_ = syllabary.first;
  std::pair<table::Index*, size_t> index =
      file()->find<table::Index>("Index");
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
    syllabary_ = file()->construct<table::Syllabary>("Syllabary", std::nothrow)(void_allocator);
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
    index_ = file()->construct<table::Index>("Index", std::nothrow)(void_allocator);
    if (!index_) {
      EZLOGGERPRINT("Error creating table index.");
      return false;
    }
    index_->resize(num_syllables);
  }
  
  for (Vocabulary::const_iterator v = vocabulary.begin(); v != vocabulary.end(); ++v) {
    const Code &code(v->first);
    const DictEntryList &ls(v->second);
    // For now only Level 1 index is supported...
    if (code.size() > 1)
      break;
    int syllable_id = code[0];
    table::IndexNode &node(index_->at(syllable_id));
    table::EntryVector *entries = NULL;
    if (node.entries) {
      // Already there?!
      entries = node.entries.get();
    }
    else {
      entries = file()->construct<table::EntryVector>(boost::interprocess::anonymous_instance)(
          void_allocator);
      if (!entries) {
        EZLOGGERPRINT("Error creating table entries; file size: %u, free memory: %u.",
                      file()->get_size(), file()->get_free_memory());
        return false;
      }
      entries->reserve(entries->size() + ls.size());
      node.entries = entries;
    }
    for (std::vector<DictEntry>::const_iterator d = ls.begin(); d != ls.end(); ++d) {
      table::Entry *entry = file()->construct<table::Entry>(boost::interprocess::anonymous_instance)(
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

const char* Table::GetSyllableById(int syllable_id) {
  if (!syllabary_ ||
      syllable_id < 0 ||
      syllable_id >= syllabary_->size())
    return NULL;
  return (*syllabary_)[syllable_id].c_str();
}

const table::EntryVector* Table::GetEntries(int syllable_id) {
  if (!index_)
    return NULL;
  if (syllable_id >= index_->size())
    return NULL;
  table::EntryVector *vec = index_->at(syllable_id).entries.get();
  if (!vec)
    return NULL;
  return vec;
}

}  // namespace rime
