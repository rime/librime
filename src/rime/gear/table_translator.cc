//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/gear/charset_filter.h>
#include <rime/gear/table_translator.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/unity_table_encoder.h>

namespace rime {

static const char* kUnitySymbol = " \xe2\x98\xaf ";

// TableTranslation

TableTranslation::TableTranslation(TranslatorOptions* options,
                                   Language* language,
                                   const string& input,
                                   size_t start, size_t end,
                                   const string& preedit)
    : options_(options), language_(language),
      input_(input), start_(start), end_(end), preedit_(preedit) {
  if (options_)
    options_->preedit_formatter().Apply(&preedit_);
  set_exhausted(true);
}

TableTranslation::TableTranslation(TranslatorOptions* options,
                                   Language* language,
                                   const string& input,
                                   size_t start, size_t end,
                                   const string& preedit,
                                   const DictEntryIterator& iter,
                                   const UserDictEntryIterator& uter)
    : options_(options), language_(language),
      input_(input), start_(start), end_(end), preedit_(preedit),
      iter_(iter), uter_(uter) {
  if (options_)
    options_->preedit_formatter().Apply(&preedit_);
  CheckEmpty();
}

bool TableTranslation::Next() {
  if (exhausted())
    return false;
  if (PreferUserPhrase()) {
    uter_.Next();
    if (uter_.exhausted())
      FetchMoreUserPhrases();
  }
  else {
    iter_.Next();
    if (iter_.exhausted())
      FetchMoreTableEntries();
  }
  return !CheckEmpty();
}

static bool is_constructed(const DictEntry* e) {
  return UnityTableEncoder::HasPrefix(e->custom_code);
}

an<Candidate> TableTranslation::Peek() {
  if (exhausted())
    return nullptr;
  bool is_user_phrase = PreferUserPhrase();
  auto e = PreferedEntry(is_user_phrase);
  string comment(is_constructed(e.get()) ? kUnitySymbol : e->comment);
  if (options_) {
    options_->comment_formatter().Apply(&comment);
  }
  auto phrase = New<Phrase>(
      language_,
      e->remaining_code_length == 0 ? "table" : "completion",
      start_, end_, e);
  if (phrase) {
    phrase->set_comment(comment);
    phrase->set_preedit(preedit_);
    bool incomplete = e->remaining_code_length != 0;
    phrase->set_quality(e->weight +
                        options_->initial_quality() +
                        (incomplete ? -1 : 0) +
                        (is_user_phrase ? 0.5 : 0));
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
      (uter_.Peek()->remaining_code_length != 0 ||
       is_constructed(uter_.Peek().get())))
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
                       const string& input,
                       size_t start, size_t end,
                       const string& preedit,
                       bool enable_user_dict);
  bool FetchUserPhrases(TableTranslator* translator);
  virtual bool FetchMoreUserPhrases();
  virtual bool FetchMoreTableEntries();

 private:
  Dictionary* dict_;
  UserDictionary* user_dict_;
  size_t limit_;
  size_t user_dict_limit_;
  string user_dict_key_;
};

LazyTableTranslation::LazyTableTranslation(TableTranslator* translator,
                                           const string& input,
                                           size_t start, size_t end,
                                           const string& preedit,
                                           bool enable_user_dict)
    : TableTranslation(translator, translator->language(),
                       input, start, end, preedit),
      dict_(translator->dict()),
      user_dict_(enable_user_dict ? translator->user_dict() : NULL),
      limit_(kInitialSearchLimit),
      user_dict_limit_(kInitialSearchLimit) {
  FetchUserPhrases(translator) || FetchMoreUserPhrases();
  FetchMoreTableEntries();
  CheckEmpty();
}

bool LazyTableTranslation::FetchUserPhrases(TableTranslator* translator) {
  if (!user_dict_)
    return false;
  // fetch all exact match entries
  user_dict_->LookupWords(&uter_, input_, false, 0, &user_dict_key_);
  auto encoder = translator->encoder();
  if (encoder && encoder->loaded()) {
    encoder->LookupPhrases(&uter_, input_, false);
  }
  return !uter_.exhausted();
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

TableTranslator::TableTranslator(const Ticket& ticket)
    : Translator(ticket),
      Memory(ticket),
      TranslatorOptions(ticket) {
  if (!engine_)
    return;
  if (Config* config = engine_->schema()->config()) {
    config->GetBool(name_space_ + "/enable_charset_filter",
                    &enable_charset_filter_);
    config->GetBool(name_space_ + "/enable_sentence",
                    &enable_sentence_);
    config->GetBool(name_space_ + "/sentence_over_completion",
                    &sentence_over_completion_);
    config->GetBool(name_space_ + "/enable_encoder",
                    &enable_encoder_);
    config->GetBool(name_space_ + "/encode_commit_history",
                    &encode_commit_history_);
    config->GetInt(name_space_ + "/max_phrase_length",
                   &max_phrase_length_);
  }
  if (enable_encoder_ && user_dict_) {
    encoder_.reset(new UnityTableEncoder(user_dict_.get()));
    encoder_->Load(ticket);
  }
}

static bool starts_with_completion(an<Translation> translation) {
  if (!translation)
    return false;
  auto cand = translation->Peek();
  return cand && cand->type() == "completion";
}

an<Translation> TableTranslator::Query(const string& input,
                                               const Segment& segment) {
  if (!segment.HasTag(tag_))
    return nullptr;
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  FinishSession();

  bool enable_user_dict = user_dict_ && user_dict_->loaded() &&
      !IsUserDictDisabledFor(input);

  const string& preedit(input);
  string code = input;
  boost::trim_right_if(code, boost::is_any_of(delimiters_));

  an<Translation> translation;
  if (enable_completion_) {
    translation = Cached<LazyTableTranslation>(
        this,
        code,
        segment.start,
        segment.start + input.length(),
        preedit,
        enable_user_dict);
  }
  else {
    DictEntryIterator iter;
    if (dict_ && dict_->loaded()) {
      dict_->LookupWords(&iter, code, false);
    }
    UserDictEntryIterator uter;
    if (enable_user_dict) {
      user_dict_->LookupWords(&uter, code, false);
      if (encoder_ && encoder_->loaded()) {
        encoder_->LookupPhrases(&uter, code, false);
      }
    }
    if (!iter.exhausted() || !uter.exhausted())
      translation = Cached<TableTranslation>(
          this,
          language(),
          code,
          segment.start,
          segment.start + input.length(),
          preedit,
          iter,
          uter);
  }
  if (translation) {
    bool filter_by_charset = enable_charset_filter_ &&
        !engine_->context()->get_option("extended_charset");
    if (filter_by_charset) {
      translation = New<CharsetFilterTranslation>(translation);
    }
  }
  if (translation && translation->exhausted()) {
    translation.reset();  // discard futile translation
  }
  if (enable_sentence_ && !translation) {
    translation = MakeSentence(input, segment.start,
                               /* include_prefix_phrases = */true);
  }
  else if (sentence_over_completion_ &&
           starts_with_completion(translation)) {
    if (auto sentence = MakeSentence(input, segment.start)) {
      translation = sentence + translation;
    }
  }
  if (translation) {
    translation = New<DistinctTranslation>(translation);
  }
  if (translation && translation->exhausted()) {
    translation.reset();  // discard futile translation
  }
  return translation;
}

bool TableTranslator::Memorize(const CommitEntry& commit_entry) {
  if (!user_dict_)
    return false;
  for (const DictEntry* e : commit_entry.elements) {
    if (is_constructed(e)) {
      DictEntry blessed(*e);
      UnityTableEncoder::RemovePrefix(&blessed.custom_code);
      user_dict_->UpdateEntry(blessed, 1);
    }
    else {
      user_dict_->UpdateEntry(*e, 1);
    }
  }
  if (encoder_ && encoder_->loaded()) {
    if (commit_entry.elements.size() > 1) {
      encoder_->EncodePhrase(commit_entry.text, "1");
    }
    if (encode_commit_history_) {
      const auto& history(engine_->context()->commit_history());
      if (!history.empty()) {
        DLOG(INFO) << "history: " << history.repr();
        auto it = history.rbegin();
        if (it->type == "punct") {  // ending with punctuation
            ++it;
        }
        string phrase;
        for (; it != history.rend(); ++it) {
          if (it->type != "table" && it->type != "sentence")
            break;
          if (phrase.empty()) {
            phrase = it->text;  // last word
            continue;
          }
          phrase = it->text + phrase;  // prepend another word
          size_t phrase_length = utf8::unchecked::distance(
              phrase.c_str(), phrase.c_str() + phrase.length());
          if (static_cast<int>(phrase_length) > max_phrase_length_)
            break;
          DLOG(INFO) << "phrase: " << phrase;
          encoder_->EncodePhrase(phrase, "0");
        }
      }
    }
  }
  return true;
}

// SentenceSyllabifier

class SentenceSyllabifier : public PhraseSyllabifier {
 public:
  virtual Spans Syllabify(const Phrase* phrase);
};

Spans SentenceSyllabifier::Syllabify(const Phrase* phrase) {
  Spans result;
  if (auto sentence = dynamic_cast<const Sentence*>(phrase)) {
    size_t stop = sentence->start();
    result.AddVertex(stop);
    for (size_t len : sentence->syllable_lengths()) {
      stop += len;
      result.AddVertex(stop);
    }
  }
  return result;
}

// SentenceTranslation

class SentenceTranslation : public Translation {
 public:
  SentenceTranslation(TableTranslator* translator,
                      an<Sentence> sentence,
                      DictEntryCollector* collector,
                      UserDictEntryCollector* ucollector,
                      const string& input,
                      size_t start);
  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  void PrepareSentence();
  bool CheckEmpty();
  bool PreferUserPhrase() const;

  TableTranslator* translator_;
  an<Sentence> sentence_;
  DictEntryCollector collector_;
  UserDictEntryCollector user_phrase_collector_;
  size_t user_phrase_index_ = 0;
  string input_;
  size_t start_;
};

SentenceTranslation::SentenceTranslation(TableTranslator* translator,
                                         an<Sentence> sentence,
                                         DictEntryCollector* collector,
                                         UserDictEntryCollector* ucollector,
                                         const string& input,
                                         size_t start)
    : translator_(translator), input_(input), start_(start) {
  sentence_.swap(sentence);
  if (collector)
    collector_.swap(*collector);
  if (ucollector)
    user_phrase_collector_.swap(*ucollector);
  PrepareSentence();
  CheckEmpty();
}

bool SentenceTranslation::Next() {
  if (sentence_) {
    sentence_.reset();
    return !CheckEmpty();
  }
  if (PreferUserPhrase()) {
    auto r = user_phrase_collector_.rbegin();
    if (++user_phrase_index_ >= r->second.size()) {
      user_phrase_collector_.erase(r->first);
      user_phrase_index_ = 0;
    }
  }
  else {
    auto r = collector_.rbegin();
    if (!r->second.Next()) {
      collector_.erase(r->first);
    }
  }
  return !CheckEmpty();
}

an<Candidate> SentenceTranslation::Peek() {
  if (exhausted())
    return nullptr;
  if (sentence_) {
    return sentence_;
  }
  size_t code_length = 0;
  an<DictEntry> entry;
  if (PreferUserPhrase()) {
    auto r = user_phrase_collector_.rbegin();
    code_length = r->first;
    entry = r->second[user_phrase_index_];
  }
  else {
    auto r = collector_.rbegin();
    code_length = r->first;
    entry = r->second.Peek();
  }
  auto result = New<Phrase>(
      translator_ ? translator_->language() : NULL,
      "table",
      start_,
      start_ + code_length,
      entry);
  if (translator_) {
    string preedit = input_.substr(0, code_length);
    translator_->preedit_formatter().Apply(&preedit);
    result->set_preedit(preedit);
  }
  return result;
}

void SentenceTranslation::PrepareSentence() {
  if (!sentence_)
    return;
  sentence_->Offset(start_);
  sentence_->set_comment(kUnitySymbol);
  sentence_->set_syllabifier(New<SentenceSyllabifier>());

  if (!translator_)
    return;
  string preedit = input_;
  const string& delimiters(translator_->delimiters());
  // split syllables
  size_t pos = 0;
  for (int len : sentence_->syllable_lengths()) {
    if (pos > 0 && delimiters.find(input_[pos - 1]) == string::npos) {
      preedit.insert(pos, 1, ' ');
      ++pos;
    }
    pos += len;
  }
  translator_->preedit_formatter().Apply(&preedit);
  sentence_->set_preedit(preedit);
}

bool SentenceTranslation::CheckEmpty() {
  set_exhausted(!sentence_ &&
                collector_.empty() &&
                user_phrase_collector_.empty());
  return exhausted();
}

bool SentenceTranslation::PreferUserPhrase() const {
  // compare code length
  int user_phrase_code_length = 0;
  int table_code_length = 0;
  if (!user_phrase_collector_.empty()) {
    user_phrase_code_length = user_phrase_collector_.rbegin()->first;
  }
  if (!collector_.empty()) {
    table_code_length = collector_.rbegin()->first;
  }
  if (user_phrase_code_length > 0 &&
      user_phrase_code_length >= table_code_length) {
    return true;
  }
  return false;
}

static size_t consume_trailing_delimiters(size_t pos,
                                          const string& input,
                                          const string& delimiters) {
  while (pos < input.length() &&
         delimiters.find(input[pos]) != string::npos) {
    ++pos;
  }
  return pos;
}

an<Translation>
TableTranslator::MakeSentence(const string& input, size_t start,
                              bool include_prefix_phrases) {
  bool filter_by_charset = enable_charset_filter_ &&
      !engine_->context()->get_option("extended_charset");
  DictEntryCollector collector;
  UserDictEntryCollector user_phrase_collector;
  map<int, an<Sentence>> sentences;
  sentences[0] = New<Sentence>(language());
  for (size_t start_pos = 0; start_pos < input.length(); ++start_pos) {
    if (sentences.find(start_pos) == sentences.end())
      continue;
    string active_input = input.substr(start_pos);
    string active_key = active_input + ' ';
    vector<of<DictEntry>> entries(active_input.length() + 1);
    // lookup dictionaries
    if (user_dict_ && user_dict_->loaded()) {
      for (size_t len = 1; len <= active_input.length(); ++len) {
        size_t consumed_length =
            consume_trailing_delimiters(len, active_input, delimiters_);
        if (entries[consumed_length])
          continue;
        DLOG(INFO) << "active input: " << active_input << "[0, " << len << ")";
        UserDictEntryIterator uter;
        string resume_key;
        string key = active_input.substr(0, len);
        user_dict_->LookupWords(&uter, key, false, 0, &resume_key);
        if (filter_by_charset) {
          uter.AddFilter(CharsetFilter::FilterDictEntry);
        }
        entries[consumed_length] = uter.Peek();
        if (start_pos == 0 && !uter.exhausted()) {
          // also provide words for manual composition
          uter.Release(&user_phrase_collector[consumed_length]);
          DLOG(INFO) << "user phrase[" << consumed_length << "]: "
                     << user_phrase_collector[consumed_length].size();
        }
        if (resume_key > active_key &&
            !boost::starts_with(resume_key, active_key))
          break;
      }
    }
    if (encoder_ && encoder_->loaded()) {
      UnityTableEncoder::AddPrefix(&active_key);
      for (size_t len = 1; len <= active_input.length(); ++len) {
        size_t consumed_length =
            consume_trailing_delimiters(len, active_input, delimiters_);
        if (entries[consumed_length])
          continue;
        DLOG(INFO) << "active input: " << active_input << "[0, " << len << ")";
        UserDictEntryIterator uter;
        string resume_key;
        string key = active_input.substr(0, len);
        encoder_->LookupPhrases(&uter, key, false, 0, &resume_key);
        if (filter_by_charset) {
          uter.AddFilter(CharsetFilter::FilterDictEntry);
        }
        entries[consumed_length] = uter.Peek();
        if (start_pos == 0 && !uter.exhausted()) {
          // also provide words for manual composition
          uter.Release(&user_phrase_collector[consumed_length]);
          DLOG(INFO) << "unity phrase[" << consumed_length << "]: "
                     << user_phrase_collector[consumed_length].size();
        }
        if (resume_key > active_key &&
            !boost::starts_with(resume_key, active_key))
          break;
      }
    }
    if (dict_ && dict_->loaded()) {
      vector<Prism::Match> matches;
      dict_->prism()->CommonPrefixSearch(input.substr(start_pos), &matches);
      if (matches.empty())
        continue;
      for (const auto& m : boost::adaptors::reverse(matches)) {
        if (m.length == 0)
          continue;
        size_t consumed_length =
            consume_trailing_delimiters(m.length, active_input, delimiters_);
        if (entries[consumed_length])
          continue;
        DictEntryIterator iter;
        dict_->LookupWords(&iter, active_input.substr(0, m.length), false);
        if (filter_by_charset) {
          iter.AddFilter(CharsetFilter::FilterDictEntry);
        }
        entries[consumed_length] = iter.Peek();
        if (start_pos == 0 && !iter.exhausted()) {
          // also provide words for manual composition
          collector[consumed_length] = iter;
          DLOG(INFO) << "table[" << consumed_length << "]: "
                     << collector[consumed_length].entry_count();
        }
      }
    }
    for (size_t len = 1; len <= active_input.length(); ++len) {
      if (!entries[len])
        continue;
      size_t end_pos = start_pos + len;
      // create a new sentence
      auto new_sentence = New<Sentence>(*sentences[start_pos]);
      new_sentence->Extend(*entries[len], end_pos);
      // compare and update sentences
      if (sentences.find(end_pos) == sentences.end() ||
          sentences[end_pos]->weight() <= new_sentence->weight()) {
        sentences[end_pos] = new_sentence;
      }
    }
  }
  an<Translation> result;
  if (sentences.find(input.length()) != sentences.end()) {
    result = Cached<SentenceTranslation>(
        this,
        sentences[input.length()],
        include_prefix_phrases ? &collector : NULL,
        include_prefix_phrases ? &user_phrase_collector : NULL,
        input,
        start);
    if (result && filter_by_charset) {
      result = New<CharsetFilterTranslation>(result);
    }
  }
  return result;
}

}  // namespace rime
