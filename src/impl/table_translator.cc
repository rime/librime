// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/impl/table_translator.h>
#include <rime/impl/translator_commons.h>

static const char* kUnityTableEncoder = " \xe2\x98\xaf ";

namespace rime {

// TableTranslation

TableTranslation::TableTranslation(TranslatorOptions* options,
                                   Language* language,
                                   const std::string &input,
                                   size_t start, size_t end)
    : options_(options), language_(language),
      input_(input), start_(start), end_(end), preedit_(input) {
  if (options_)
    options_->preedit_formatter().Apply(&preedit_);
  set_exhausted(true);
}

TableTranslation::TableTranslation(TranslatorOptions* options,
                                   Language* language,
                                   const std::string &input,
                                   size_t start, size_t end,
                                   const DictEntryIterator& iter,
                                   const UserDictEntryIterator& uter)
    : options_(options), language_(language),
      input_(input), start_(start), end_(end), preedit_(input),
      iter_(iter), uter_(uter) {
  if (options_)
    options_->preedit_formatter().Apply(&preedit_);
  CheckEmpty();
}

bool TableTranslation::Next() {
  if (exhausted())
    return false;
  do {
    bool prefer_user_phrase = PreferUserPhrase();
    candidate_set_.insert(PreferedEntry(prefer_user_phrase)->text);
    if (prefer_user_phrase) {
      uter_.Next();
      if (uter_.exhausted())
        FetchMoreUserPhrases();
    }
    else {
      iter_.Next();
      if (iter_.exhausted())
        FetchMoreTableEntries();
    }
    CheckEmpty();
  }
  while (!exhausted() && /* skip duplicate candidates */
         HasCandidate(PreferedEntry(PreferUserPhrase())->text));
  return !exhausted();
}

shared_ptr<Candidate> TableTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e = PreferedEntry(PreferUserPhrase());
  std::string comment(e->comment);
  if (options_) {
    options_->comment_formatter().Apply(&comment);
  }
  shared_ptr<Phrase> phrase = boost::make_shared<Phrase>(
      language_,
      e->remaining_code_length == 0 ? "table" : "completion",
      start_, end_, e);
  if (phrase) {
    phrase->set_comment(comment);
    phrase->set_preedit(preedit_);
  }
  return phrase;
}

bool TableTranslation::CheckEmpty() {
  bool is_empty = iter_.exhausted() && uter_.exhausted();
  set_exhausted(is_empty);
  return is_empty;
}

bool TableTranslation::PreferUserPhrase() {
  if (uter_.exhausted())
    return false;
  if (iter_.exhausted())
    return true;
  if (iter_.Peek()->remaining_code_length == 0 &&
      uter_.Peek()->remaining_code_length != 0)
    return false;
  else
    return true;
}

// LazyTableTranslation

class LazyTableTranslation : public TableTranslation {
 public:
  static const size_t kInitialSearchLimit = 10;
  static const size_t kExpandingFactor = 10;
  
  LazyTableTranslation(TableTranslator* translator,
                       const std::string &input,
                       size_t start, size_t end,
                       bool enable_user_dict);
  virtual bool FetchMoreUserPhrases();
  virtual bool FetchMoreTableEntries();
  
 private:
  Dictionary* dict_;
  UserDictionary* user_dict_;
  size_t limit_;
  size_t user_dict_limit_;
  std::string user_dict_key_;
};

LazyTableTranslation::LazyTableTranslation(TableTranslator* translator,
                                           const std::string &input,
                                           size_t start, size_t end,
                                           bool enable_user_dict)
    : TableTranslation(translator,
                       translator->language(),
                       input, start, end),
      dict_(translator->dict()),
      user_dict_(enable_user_dict ? translator->user_dict() : NULL),
      limit_(kInitialSearchLimit), user_dict_limit_(kInitialSearchLimit) {
  FetchMoreUserPhrases();
  FetchMoreTableEntries();
  CheckEmpty();
}

bool LazyTableTranslation::FetchMoreUserPhrases() {
  if (!user_dict_ || user_dict_limit_ == 0)
    return false;
  size_t count = user_dict_->LookupWords(&uter_, input_, true,
                                         user_dict_limit_, &user_dict_key_);
  if (count < user_dict_limit_) {
    DLOG(INFO) << "all user dict entries obtained.";
    user_dict_limit_ = 0;  // no more try
  }
  else {
    user_dict_limit_ *= kExpandingFactor;
  }
  return !uter_.exhausted();
}

bool LazyTableTranslation::FetchMoreTableEntries() {
  if (!dict_ || limit_ == 0)
    return false;
  size_t previous_entry_count = iter_.entry_count();
  DLOG(INFO) << "fetching more table entries: limit = " << limit_
             << ", count = " << previous_entry_count;
  DictEntryIterator more;
  if (dict_->LookupWords(&more, input_, true, limit_) < limit_) {
    DLOG(INFO) << "all table entries obtained.";
    limit_ = 0;  // no more try
  }
  else {
    limit_ *= kExpandingFactor;
  }
  if (more.entry_count() > previous_entry_count) {
    more.Skip(previous_entry_count);
    iter_ = more;
  }
  return true;
}

// TableTranslator

TableTranslator::TableTranslator(Engine *engine)
    : Translator(engine),
      Memory(engine),
      TranslatorOptions(engine),
      enable_charset_filter_(false) {
  if (!engine) return;
  Config *config = engine->schema()->config();
  if (config) {
    config->GetBool("translator/enable_charset_filter",
                    &enable_charset_filter_);
  }
}

TableTranslator::~TableTranslator() {
}

shared_ptr<Translation> TableTranslator::Query(const std::string &input,
                                               const Segment &segment,
                                               std::string* prompt) {
  if (!dict_ || !dict_->loaded())
    return shared_ptr<Translation>();
  if (!segment.HasTag("abc"))
    return shared_ptr<Translation>();
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  bool enable_user_dict = true;
  if (!user_dict_disabling_patterns_.empty()) {
    BOOST_FOREACH(const boost::regex& pattern, user_dict_disabling_patterns_) {
      if (boost::regex_match(input, pattern)) {
        enable_user_dict = false;
        break;
      }
    }
  }

  std::string code(input);
  boost::trim_right_if(code, boost::is_any_of(delimiters_));
  
  shared_ptr<Translation> translation;
  if (enable_completion_) {
    translation = boost::make_shared<LazyTableTranslation>(
        this,
        code,
        segment.start,
        segment.start + input.length(),
        enable_user_dict);
  }
  else {
    DictEntryIterator iter;
    dict_->LookupWords(&iter, code, false);
    UserDictEntryIterator uter;
    if (user_dict_ && enable_user_dict) {
      user_dict_->LookupWords(&uter, code, false);
    }
    if (!iter.exhausted() || !uter.exhausted())
      translation = boost::make_shared<TableTranslation>(
          this,
          language(),
          code,
          segment.start,
          segment.start + input.length(),
          iter,
          uter);
  }

  if (translation) {
    bool filter_by_charset = enable_charset_filter_ &&
        !engine_->context()->get_option("extended_charset");
    if (filter_by_charset) {
      translation = make_shared<CharsetFilter>(translation);
    }
  }
  if (!translation || translation->exhausted()) {
    translation = MakeSentence(input, segment.start);
  }
  return translation;
}

bool TableTranslator::Memorize(const DictEntry& commit_entry,
                               const std::vector<const DictEntry*>& elements) {
  if (!user_dict_) return false;
  BOOST_FOREACH(const DictEntry* e, elements) {
    user_dict_->UpdateEntry(*e, 1);
  }
  // TODO
  return true;
}

// SentenceTranslation

class SentenceTranslation : public Translation {
 public:
  SentenceTranslation(TableTranslator* translator,
                      shared_ptr<Sentence> sentence,
                      DictEntryCollector collector,
                      const std::string& input,
                      size_t start)
      : translator_(translator), input_(input), start_(start) {
    sentence_.swap(sentence);
    collector_.swap(collector);
    PrepareSentence();
    set_exhausted(!sentence_ && collector_.empty());
  }

  bool Next() {
    if (sentence_) {
      sentence_.reset();
    }
    else if (!collector_.empty()) {
      DictEntryCollector::reverse_iterator r(collector_.rbegin());
      if (!r->second.Next()) {
        collector_.erase(r->first);
      }
    }
    set_exhausted(!sentence_ && collector_.empty());
    return !exhausted();
  }
  
  shared_ptr<Candidate> Peek() {
    if (sentence_) {
      return sentence_;
    }
    if (collector_.empty()) {
      return shared_ptr<Candidate>();
    }
    DictEntryCollector::reverse_iterator r(collector_.rbegin());
    shared_ptr<DictEntry> entry(r->second.Peek());
    shared_ptr<Phrase> result = boost::make_shared<Phrase>(
        translator_ ? translator_->language() : NULL,
        "table",
        start_,
        start_ + r->first,
        entry);
    if (translator_) {
      std::string preedit(input_.substr(0, r->first));
      translator_->preedit_formatter().Apply(&preedit);
      result->set_preedit(preedit);
    }
    return result;
  }
  
 protected:
  void PrepareSentence();
  
  TableTranslator* translator_;
  shared_ptr<Sentence> sentence_;
  DictEntryCollector collector_;
  std::string input_;
  size_t start_;
};

void SentenceTranslation::PrepareSentence() {
  if (!sentence_) return;
  sentence_->Offset(start_);
  sentence_->set_comment(kUnityTableEncoder);
  
  if (!translator_) return;
  std::string preedit(input_);
  const std::string& delimiters(translator_->delimiters());
  // split syllables
  size_t pos = 0;
  BOOST_FOREACH(int len, sentence_->syllable_lengths()) {
    if (pos > 0 &&
        delimiters.find(input_[pos - 1]) == std::string::npos) {
      preedit.insert(pos, 1, ' ');
      ++pos;
    }
    pos += len;
  }
  translator_->preedit_formatter().Apply(&preedit);
  sentence_->set_preedit(preedit);
}

shared_ptr<Translation> TableTranslator::MakeSentence(const std::string& input,
                                                      size_t start) {
  bool filter_by_charset = enable_charset_filter_ &&
      !engine_->context()->get_option("extended_charset");
  DictEntryCollector collector;
  std::map<int, shared_ptr<Sentence> > sentences;
  sentences[0] = make_shared<Sentence>(language());
  for (size_t start_pos = 0; start_pos < input.length(); ++start_pos) {
    if (sentences.find(start_pos) == sentences.end())
      continue;
    std::vector<Prism::Match> matches;
    dict_->prism()->CommonPrefixSearch(input.substr(start_pos), &matches);
    if (matches.empty()) continue;
    BOOST_REVERSE_FOREACH(const Prism::Match &m, matches) {
      if (m.length == 0) continue;
      DictEntryIterator iter;
      dict_->LookupWords(&iter, input.substr(start_pos, m.length), false);
      if (iter.exhausted()) continue;
      size_t end_pos = start_pos + m.length;
      // consume trailing delimiters
      while (end_pos < input.length() &&
             delimiters_.find(input[end_pos]) != std::string::npos)
        ++end_pos;
      shared_ptr<Sentence> new_sentence =
          make_shared<Sentence>(*sentences[start_pos]);
      // extend the sentence with the first suitable entry
      shared_ptr<DictEntry> entry = iter.Peek();
      if (filter_by_charset) {
        while (!CharsetFilter::Passed(entry->text) &&
               iter.Next()) {
          entry = iter.Peek();
        }
        if (iter.exhausted()) continue;
      }
      new_sentence->Extend(*entry, end_pos);
      // compare and update sentences
      if (sentences.find(end_pos) == sentences.end() ||
          sentences[end_pos]->weight() <= new_sentence->weight()) {
        sentences[end_pos] = new_sentence;
      }
      // also provide words for manual composition
      if (start_pos == 0) {
        collector[end_pos] = iter;
      }
    }
  }
  shared_ptr<Translation> result;
  if (sentences.find(input.length()) != sentences.end()) {
    result = boost::make_shared<SentenceTranslation>(this,
                                                     sentences[input.length()],
                                                     collector,
                                                     input,
                                                     start);
    if (result && filter_by_charset) {
      result = make_shared<CharsetFilter>(result);
    }
  }
  return result;
}

}  // namespace rime
