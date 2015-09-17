//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/ticket.h>
#include <rime/algo/dynamics.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/db.h>
#include <rime/dict/table.h>
#include <rime/dict/user_dictionary.h>

namespace rime {

struct DfsState {
  size_t depth_limit;
  TickCount present_tick;
  Code code;
  vector<double> credibility;
  an<UserDictEntryCollector> collector;
  an<DbAccessor> accessor;
  string key;
  string value;

  bool IsExactMatch(const string& prefix) {
    return boost::starts_with(key, prefix + '\t');
  }
  bool IsPrefixMatch(const string& prefix) {
    return boost::starts_with(key, prefix);
  }
  void RecruitEntry(size_t pos);
  bool NextEntry() {
    if (!accessor->GetNextRecord(&key, &value)) {
      key.clear();
      value.clear();
      return false;  // reached the end
    }
    return true;
  }
  bool ForwardScan(const string& prefix) {
    if (!accessor->Jump(prefix)) {
      return false;
    }
    return NextEntry();
  }
  bool Backdate(const string& prefix) {
    DLOG(INFO) << "backdate; prefix: " << prefix;
    if (!accessor->Reset()) {
      LOG(WARNING) << "backdating failed for '" << prefix << "'.";
      return false;
    }
    return NextEntry();
  }
};

void DfsState::RecruitEntry(size_t pos) {
  auto e = UserDictionary::CreateDictEntry(key, value, present_tick,
                                           credibility.back());
  if (e) {
    e->code = code;
    DLOG(INFO) << "add entry at pos " << pos;
    (*collector)[pos].push_back(e);
  }
}

// UserDictEntryIterator members

void UserDictEntryIterator::Add(const an<DictEntry>& entry) {
  if (!entries_) {
    entries_ = New<DictEntryList>();
  }
  entries_->push_back(entry);
}

void UserDictEntryIterator::SortRange(size_t start, size_t count) {
  if (entries_)
    entries_->SortRange(start, count);
}

bool UserDictEntryIterator::Release(DictEntryList* receiver) {
  if (!entries_)
    return false;
  if (receiver)
    entries_->swap(*receiver);
  entries_.reset();
  index_ = 0;
  return true;
}

an<DictEntry> UserDictEntryIterator::Peek() {
  an<DictEntry> result;
  while (!result && !exhausted()) {
    result = (*entries_)[index_];
    if (filter_ && !filter_(result)) {
      ++index_;
      result.reset();
    }
  }
  return result;
}

bool UserDictEntryIterator::Next() {
  if (exhausted())
    return false;
  ++index_;
  return exhausted();
}

// UserDictionary members

UserDictionary::UserDictionary(const an<Db>& db)
    : db_(db) {
}

UserDictionary::~UserDictionary() {
  if (loaded()) {
    CommitPendingTransaction();
  }
}

void UserDictionary::Attach(const an<Table>& table,
                            const an<Prism>& prism) {
  table_ = table;
  prism_ = prism;
}

bool UserDictionary::Load() {
  if (!db_)
    return false;
  if (!db_->loaded() && !db_->Open()) {
    // try to recover managed db in available work thread
    Deployer& deployer(Service::instance().deployer());
    auto task = DeploymentTask::Require("userdb_recovery_task");
    if (task && Is<Recoverable>(db_) && !deployer.IsWorking()) {
      deployer.ScheduleTask(an<DeploymentTask>(task->Create(db_)));
      deployer.StartWork();
    }
    return false;
  }
  if (!FetchTickCount() && !Initialize())
    return false;
  return true;
}

bool UserDictionary::loaded() const {
  return db_ && !db_->disabled() && db_->loaded();
}

bool UserDictionary::readonly() const {
  return db_ && db_->readonly();
}

// this is a one-pass scan for the user db which supports sequential access
// in alphabetical order (of syllables).
// each call to DfsLookup() searches for matching phrases at a given
// start position: current_pos.
// there may be multiple edges that start at current_pos, and ends at different
// positions after current_pos. on each edge, there can be multiple syllables
// the spelling on the edge maps to.
// in order to enable forward scaning and to avoid backdating, our strategy is:
// sort all those syllables from edges that starts at current_pos, so that
// the syllables are in the same alphabetical order as the user db's.
// this having been done by transposing the syllable graph into
// SyllableGraph::index.
// however, in the case of 'shsh' which could be the abbreviation of either
// 'sh(a) sh(i)' or 'sh(a) s(hi) h(ou)',
// we now have to give up the latter path in order to avoid backdating.

// update: 2013-06-25
// to fix the following issue, we have to reintroduce backdating in db scan:
// given aaa=A, b=B, ab=C, derive/^(aa)a$/$1/,
// the input 'aaab' can be either aaa'b=AB or aa'ab=AC.
// note that backdating works only for normal or fuzzy spellings, but not for
// abbreviations such as 'shsh' in the previous example.

void UserDictionary::DfsLookup(const SyllableGraph& syll_graph,
                               size_t current_pos,
                               const string& current_prefix,
                               DfsState* state) {
  auto index = syll_graph.indices.find(current_pos);
  if (index == syll_graph.indices.end()) {
    return;
  }
  DLOG(INFO) << "dfs lookup starts from " << current_pos;
  string prefix;
  for (const auto& spelling : index->second) {
    DLOG(INFO) << "prefix: '" << current_prefix << "'"
               << ", syll_id: " << spelling.first
               << ", num_spellings: " << spelling.second.size();
    state->code.push_back(spelling.first);
    BOOST_SCOPE_EXIT( (&state) ) {
      state->code.pop_back();
    }
    BOOST_SCOPE_EXIT_END
    if (!TranslateCodeToString(state->code, &prefix))
      continue;
    for (size_t i = 0; i < spelling.second.size(); ++i) {
      auto props = spelling.second[i];
      if (i > 0 && props->type >= kAbbreviation)
        continue;
      state->credibility.push_back(
          state->credibility.back() * props->credibility);
      BOOST_SCOPE_EXIT( (&state) ) {
        state->credibility.pop_back();
      }
      BOOST_SCOPE_EXIT_END
      size_t end_pos = props->end_pos;
      DLOG(INFO) << "edge: [" << current_pos << ", " << end_pos << ")";
      if (prefix != state->key) {  // 'a b c |d ' > 'a b c \tabracadabra'
        DLOG(INFO) << "forward scanning for '" << prefix << "'.";
        if (!state->ForwardScan(prefix))  // reached the end of db
          continue;
      }
      while (state->IsExactMatch(prefix)) {  // 'b |e ' vs. 'b e \tBe'
        DLOG(INFO) << "match found for '" << prefix << "'.";
        state->RecruitEntry(end_pos);
        if (!state->NextEntry())  // reached the end of db
          break;
      }
      // the caller can limit the number of syllables to look up
      if ((!state->depth_limit || state->code.size() < state->depth_limit) &&
          state->IsPrefixMatch(prefix)) {  // 'b |e ' vs. 'b e f \tBefore'
        DfsLookup(syll_graph, end_pos, prefix, state);
      }
    }
    if (!state->IsPrefixMatch(current_prefix))  // 'b |' vs. 'g o \tGo'
      return;
    // 'b |e ' vs. 'b y \tBy'
  }
}

an<UserDictEntryCollector>
UserDictionary::Lookup(const SyllableGraph& syll_graph,
                       size_t start_pos,
                       size_t depth_limit,
                       double initial_credibility) {
  if (!table_ || !prism_ || !loaded() ||
      start_pos >= syll_graph.interpreted_length)
    return nullptr;
  DfsState state;
  state.depth_limit = depth_limit;
  FetchTickCount();
  state.present_tick = tick_ + 1;
  state.credibility.push_back(initial_credibility);
  state.collector = New<UserDictEntryCollector>();
  state.accessor = db_->Query("");
  state.accessor->Jump(" ");  // skip metadata
  string prefix;
  DfsLookup(syll_graph, start_pos, prefix, &state);
  if (state.collector->empty())
    return nullptr;
  // sort each group of homophones by weight
  for (auto& v : *state.collector) {
    v.second.Sort();
  }
  return state.collector;
}

size_t UserDictionary::LookupWords(UserDictEntryIterator* result,
                                   const string& input,
                                   bool predictive,
                                   size_t limit,
                                   string* resume_key) {
  TickCount present_tick = tick_ + 1;
  size_t len = input.length();
  size_t start = result->size();
  size_t count = 0;
  size_t exact_match_count = 0;
  const string kEnd = "\xff";
  string key;
  string value;
  string full_code;
  auto accessor = db_->Query(input);
  if (!accessor || accessor->exhausted()) {
    if (resume_key)
      *resume_key = kEnd;
    return 0;
  }
  if (resume_key && !resume_key->empty()) {
    if (!accessor->Jump(*resume_key) ||
        !accessor->GetNextRecord(&key, &value)) {
      *resume_key = kEnd;
      return 0;
    }
    DLOG(INFO) << "resume lookup after: " << key;
  }
  string last_key(key);
  while (accessor->GetNextRecord(&key, &value)) {
    DLOG(INFO) << "key : " << key << ", value: " << value;
    bool is_exact_match = (len < key.length() && key[len] == ' ');
    if (!is_exact_match && !predictive) {
      key = last_key;
      break;
    }
    last_key = key;
    auto e = CreateDictEntry(key, value, present_tick, 1.0, &full_code);
    if (!e)
      continue;
    e->custom_code = full_code;
    boost::trim_right(full_code);  // remove trailing space a user dict key has
    if (full_code.length() > len) {
      e->comment = "~" + full_code.substr(len);
      e->remaining_code_length = full_code.length() - len;
    }
    result->Add(e);
    ++count;
    if (is_exact_match)
      ++exact_match_count;
    else if (limit && count >= limit)
      break;
  }
  if (exact_match_count > 0) {
    result->SortRange(start, exact_match_count);
  }
  if (resume_key) {
    *resume_key = key;
    DLOG(INFO) << "resume key reset to: " << *resume_key;
  }
  return count;
}

bool UserDictionary::UpdateEntry(const DictEntry& entry, int commits) {
  return UpdateEntry(entry, commits, "");
}

bool UserDictionary::UpdateEntry(const DictEntry& entry, int commits,
                                 const string& new_entry_prefix) {
  string code_str(entry.custom_code);
  if (code_str.empty() && !TranslateCodeToString(entry.code, &code_str))
    return false;
  string key(code_str + '\t' + entry.text);
  string value;
  UserDbValue v;
  if (db_->Fetch(key, &value)) {
    v.Unpack(value);
    if (v.tick > tick_) {
      v.tick = tick_;  // fix abnormal timestamp
    }
  }
  else if (!new_entry_prefix.empty()) {
    key.insert(0, new_entry_prefix);
  }
  if (commits > 0) {
    if (v.commits < 0)
      v.commits = -v.commits;  // revive a deleted item
    v.commits += commits;
    UpdateTickCount(1);
    v.dee = algo::formula_d(commits, (double)tick_, v.dee, (double)v.tick);
  }
  else if (commits == 0) {
    const double k = 0.1;
    v.dee = algo::formula_d(k, (double)tick_, v.dee, (double)v.tick);
  }
  else if (commits < 0) {  // mark as deleted
    v.commits = (std::min)(-1, -v.commits);
    v.dee = algo::formula_d(0.0, (double)tick_, v.dee, (double)v.tick);
  }
  v.tick = tick_;
  return db_->Update(key, v.Pack());
}

bool UserDictionary::UpdateTickCount(TickCount increment) {
  tick_ += increment;
  try {
    return db_->MetaUpdate("/tick", boost::lexical_cast<string>(tick_));
  }
  catch (...) {
    return false;
  }
}

bool UserDictionary::Initialize() {
  return db_->MetaUpdate("/tick", "0");
}

bool UserDictionary::FetchTickCount() {
  string value;
  try {
    // an earlier version mistakenly wrote tick count into an empty key
    if (!db_->MetaFetch("/tick", &value) &&
        !db_->Fetch("", &value))
      return false;
    tick_ = boost::lexical_cast<TickCount>(value);
    return true;
  }
  catch (...) {
    //tick_ = 0;
    return false;
  }
}

bool UserDictionary::NewTransaction() {
  auto db = As<Transactional>(db_);
  if (!db)
    return false;
  CommitPendingTransaction();
  transaction_time_ = time(NULL);
  return db->BeginTransaction();
}

bool UserDictionary::RevertRecentTransaction() {
  auto db = As<Transactional>(db_);
  if (!db || !db->in_transaction())
    return false;
  if (time(NULL) - transaction_time_ > 3/*seconds*/)
    return false;
  return db->AbortTransaction();
}

bool UserDictionary::CommitPendingTransaction() {
  auto db = As<Transactional>(db_);
  if (db && db->in_transaction()) {
    return db->CommitTransaction();
  }
  return false;
}

bool UserDictionary::TranslateCodeToString(const Code& code,
                                           string* result) {
  if (!table_ || !result) return false;
  result->clear();
  for (const SyllableId& syllable_id : code) {
    string spelling = table_->GetSyllableById(syllable_id);
    if (spelling.empty()) {
      LOG(ERROR) << "Error translating syllable_id '" << syllable_id << "'.";
      result->clear();
      return false;
    }
    *result += spelling;
    *result += ' ';
  }
  return true;
}

an<DictEntry> UserDictionary::CreateDictEntry(const string& key,
                                                      const string& value,
                                                      TickCount present_tick,
                                                      double credibility,
                                                      string* full_code) {
  an<DictEntry> e;
  size_t separator_pos = key.find('\t');
  if (separator_pos == string::npos)
    return e;
  UserDbValue v;
  if (!v.Unpack(value))
    return e;
  if (v.commits < 0)  // deleted entry
    return e;
  if (v.tick < present_tick)
    v.dee = algo::formula_d(0, (double)present_tick, v.dee, (double)v.tick);
  // create!
  e = New<DictEntry>();
  e->text = key.substr(separator_pos + 1);
  e->commit_count = v.commits;
  // TODO: argument s not defined...
  e->weight = algo::formula_p(0,
                              (double)v.commits / present_tick,
                              (double)present_tick,
                              v.dee) * credibility;
  if (full_code) {
    *full_code = key.substr(0, separator_pos);
  }
  DLOG(INFO) << "text = '" << e->text
             << "', code_len = " << e->code.size()
             << ", weight = " << e->weight
             << ", commit_count = " << e->commit_count
             << ", present_tick = " << present_tick;
  return e;
}

// UserDictionaryComponent members

UserDictionaryComponent::UserDictionaryComponent() {
}

UserDictionary* UserDictionaryComponent::Create(const Ticket& ticket) {
  if (!ticket.schema)
    return NULL;
  Config* config = ticket.schema->config();
  bool enable_user_dict = true;
  config->GetBool(ticket.name_space + "/enable_user_dict", &enable_user_dict);
  if (!enable_user_dict)
    return NULL;
  string dict_name;
  if (config->GetString(ticket.name_space + "/user_dict", &dict_name)) {
    // user specified name
  }
  else if (config->GetString(ticket.name_space + "/dictionary", &dict_name)) {
    // {dictionary: lunapinyin.extra} implies {user_dict: luna_pinyin}
    size_t dot = dict_name.find('.');
    if (dot != string::npos && dot != 0)
      dict_name.resize(dot);
  }
  else {
    LOG(ERROR) << ticket.name_space << "/dictionary not specified in schema '"
               << ticket.schema->schema_id() << "'.";
    return NULL;
  }
  string db_class("userdb");
  if (config->GetString(ticket.name_space + "/db_class", &db_class)) {
    // user specified db class
  }
  // obtain userdb object
  auto db = db_pool_[dict_name].lock();
  if (!db) {
    auto component = Db::Require(db_class);
    if (!component) {
      LOG(ERROR) << "undefined db class '" << db_class << "'.";
      return NULL;
    }
    db.reset(component->Create(dict_name));
    db_pool_[dict_name] = db;
  }
  return new UserDictionary(db);
}

}  // namespace rime
