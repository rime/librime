//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/service.h>
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
  std::vector<double> credibility;
  shared_ptr<UserDictEntryCollector> collector;
  shared_ptr<DbAccessor> accessor;
  std::string key;
  std::string value;

  bool IsExactMatch(const std::string &prefix) {
    return boost::starts_with(key, prefix + '\t');
  }
  bool IsPrefixMatch(const std::string &prefix) {
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
  bool ForwardScan(const std::string &prefix) {
    if (!accessor->Jump(prefix)) {
      return false;
    }
    return NextEntry();
  }
  bool Backdate(const std::string &prefix) {
    DLOG(INFO) << "backdate; prefix: " << prefix;
    if (!accessor->Reset()) {
      LOG(WARNING) << "backdating failed for '" << prefix << "'.";
      return false;
    }
    return NextEntry();
  }
};

void DfsState::RecruitEntry(size_t pos) {
  shared_ptr<DictEntry> e =
      UserDictionary::CreateDictEntry(key, value, present_tick,
                                      credibility.back());
  if (e) {
    e->code = code;
    DLOG(INFO) << "add entry at pos " << pos;
    (*collector)[pos].push_back(e);
  }
}

class UserDbRecoveryTask : public DeploymentTask {
 public:
  explicit UserDbRecoveryTask(shared_ptr<Db> db) : db_(db) {}
  bool Run(Deployer* deployer);
 protected:
  shared_ptr<Db> db_;
};

bool UserDbRecoveryTask::Run(Deployer* deployer) {
  bool success = As<Recoverable>(db_)->Recover();
  db_->enable();
  return success;
}

// UserDictEntryIterator members

void UserDictEntryIterator::Add(const shared_ptr<DictEntry>& entry) {
  if (!entries_) {
    entries_ = make_shared<DictEntryList>();
  }
  entries_->push_back(entry);
}

void UserDictEntryIterator::SortN(size_t count) {
  if (entries_)
    entries_->SortN(count);
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


shared_ptr<DictEntry> UserDictEntryIterator::Peek() {
  if (exhausted())
    return shared_ptr<DictEntry>();
  return (*entries_)[index_];
}

bool UserDictEntryIterator::Next() {
  if (exhausted())
    return false;
  ++index_;
  return exhausted();
}

// UserDictionary members

UserDictionary::UserDictionary(const shared_ptr<Db> &db)
    : db_(db), tick_(0), transaction_time_(0) {
}

UserDictionary::~UserDictionary() {
  if (loaded()) {
    CommitPendingTransaction();
  }
}

void UserDictionary::Attach(const shared_ptr<Table> &table,
                            const shared_ptr<Prism> &prism) {
  table_ = table;
  prism_ = prism;
}

bool UserDictionary::Load() {
  if (!db_)
    return false;
  if (!db_->Open()) {
    // try to recover managed db in available work thread
    Deployer& deployer(Service::instance().deployer());
    if (Is<Recoverable>(db_) && !deployer.IsWorking()) {
      db_->disable();
      deployer.ScheduleTask(New<UserDbRecoveryTask>(db_));
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

void UserDictionary::DfsLookup(const SyllableGraph &syll_graph,
                               size_t current_pos,
                               const std::string &current_prefix,
                               DfsState *state) {
  SpellingIndices::const_iterator index = syll_graph.indices.find(current_pos);
  if (index == syll_graph.indices.end()) {
    return;
  }
  DLOG(INFO) << "dfs lookup starts from " << current_pos;
  std::string prefix;
  BOOST_FOREACH(const SpellingIndex::value_type& spelling, index->second) {
    if (spelling.second.empty()) continue;
    const SpellingProperties* props = spelling.second[0];
    size_t end_pos = props->end_pos;
    DLOG(INFO) << "prefix: '" << current_prefix
               << "', syll_id: " << spelling.first
               << ", edge: [" << current_pos << ", " << end_pos << ") of "
               << spelling.second.size();
    state->code.push_back(spelling.first);
    state->credibility.push_back(
        state->credibility.back() * props->credibility);
    BOOST_SCOPE_EXIT( (&state) ) {
      state->code.pop_back();
      state->credibility.pop_back();
    } BOOST_SCOPE_EXIT_END
    if (!TranslateCodeToString(state->code, &prefix))
      continue;
    if (prefix > state->key) {  // 'a b c |d ' > 'a b c \tabracadabra'
      DLOG(INFO) << "forward scanning for '" << prefix << "'.";
      if (!state->ForwardScan(prefix))  // reached the end of db
        return;
    }
    while (state->IsExactMatch(prefix)) {  // 'b |e ' vs. 'b e \tBe'
      DLOG(INFO) << "match found for '" << prefix << "'.";
      state->RecruitEntry(end_pos);
      if (!state->NextEntry())  // reached the end of db
        return;
    }
    // the caller can limit the number of syllables to look up
    if ((!state->depth_limit || state->code.size() < state->depth_limit)
        && state->IsPrefixMatch(prefix)) {  // 'b |e ' vs. 'b e f \tBefore'
      DfsLookup(syll_graph, end_pos, prefix, state);
    }
    if (!state->IsPrefixMatch(current_prefix))  // 'b |' vs. 'g o \tGo'
      return;
    // 'b |e ' vs. 'b y \tBy'
  }
}

shared_ptr<UserDictEntryCollector>
UserDictionary::Lookup(const SyllableGraph &syll_graph,
                       size_t start_pos,
                       size_t depth_limit,
                       double initial_credibility) {
  if (!table_ || !prism_ || !loaded() ||
      start_pos >= syll_graph.interpreted_length)
    return shared_ptr<UserDictEntryCollector>();
  DfsState state;
  state.depth_limit = depth_limit;
  FetchTickCount();
  state.present_tick = tick_ + 1;
  state.credibility.push_back(initial_credibility);
  state.collector = make_shared<UserDictEntryCollector>();
  state.accessor = db_->Query("");
  state.accessor->Jump(" ");  // skip metadata
  std::string prefix;
  DfsLookup(syll_graph, start_pos, prefix, &state);
  if (state.collector->empty())
    return shared_ptr<UserDictEntryCollector>();
  // sort each group of homophones by weight
  BOOST_FOREACH(UserDictEntryCollector::value_type &v, *state.collector) {
    v.second.Sort();
  }
  return state.collector;
}

size_t UserDictionary::LookupWords(UserDictEntryIterator* result,
                                   const std::string& input,
                                   bool predictive,
                                   size_t limit,
                                   std::string* resume_key) {
  TickCount present_tick = tick_ + 1;
  size_t len = input.length();
  size_t count = 0;
  size_t exact_match_count = 0;
  const std::string kEnd = "\xff";
  std::string key;
  std::string value;
  std::string full_code;
  shared_ptr<DbAccessor> a = db_->Query(input);
  if (!a || a->exhausted()) {
    if (resume_key)
      *resume_key = kEnd;
    return 0;
  }
  if (resume_key && !resume_key->empty()) {
    if (!a->Jump(*resume_key) ||
        !a->GetNextRecord(&key, &value)) {
      *resume_key = kEnd;
      return 0;
    }
    DLOG(INFO) << "resume lookup after: " << key;
  }
  while (a->GetNextRecord(&key, &value)) {
    bool is_exact_match = (len < key.length() && key[len] == ' ');
    if (!is_exact_match && !predictive)
      break;
    shared_ptr<DictEntry> e =
        CreateDictEntry(key, value, present_tick, 1.0, &full_code);
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
    result->SortN(exact_match_count);
  }
  if (resume_key) {
    *resume_key = key;
    DLOG(INFO) << "resume key reset to: " << *resume_key;
  }
  return count;
}

bool UserDictionary::UpdateEntry(const DictEntry &entry, int commits) {
  std::string code_str(entry.custom_code);
  if (code_str.empty() && !TranslateCodeToString(entry.code, &code_str))
    return false;
  std::string key(code_str + '\t' + entry.text);
  std::string value;
  UserDbValue v;
  if (db_->Fetch(key, &value)) {
    v.Unpack(value);
    if (v.tick > tick_) {
      v.tick = tick_;  // fix abnormal timestamp
    }
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
    return db_->MetaUpdate("/tick", boost::lexical_cast<std::string>(tick_));
  }
  catch (...) {
    return false;
  }
}

bool UserDictionary::Initialize() {
  return db_->MetaUpdate("/tick", "0");
}

bool UserDictionary::FetchTickCount() {
  std::string value;
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
  shared_ptr<Transactional> db = As<Transactional>(db_);
  if (!db)
    return false;
  CommitPendingTransaction();
  transaction_time_ = time(NULL);
  return db->BeginTransaction();
}

bool UserDictionary::RevertRecentTransaction() {
  shared_ptr<Transactional> db = As<Transactional>(db_);
  if (!db || !db->in_transaction())
    return false;
  if (time(NULL) - transaction_time_ > 3/*seconds*/)
    return false;
  return db->AbortTransaction();
}

bool UserDictionary::CommitPendingTransaction() {
  shared_ptr<Transactional> db = As<Transactional>(db_);
  if (db && db->in_transaction()) {
    return db->CommitTransaction();
  }
  return false;
}

bool UserDictionary::TranslateCodeToString(const Code &code,
                                           std::string* result) {
  if (!table_ || !result) return false;
  result->clear();
  BOOST_FOREACH(const int &syllable_id, code) {
    const char *spelling = table_->GetSyllableById(syllable_id);
    if (!spelling) {
      LOG(ERROR) << "Error translating syllable_id '" << syllable_id << "'.";
      result->clear();
      return false;
    }
    *result += spelling;
    *result += ' ';
  }
  return true;
}

shared_ptr<DictEntry> UserDictionary::CreateDictEntry(const std::string& key,
                                                      const std::string& value,
                                                      TickCount present_tick,
                                                      double credibility,
                                                      std::string* full_code) {
  shared_ptr<DictEntry> e;
  size_t separator_pos = key.find('\t');
  if (separator_pos == std::string::npos)
    return e;
  UserDbValue v;
  if (!v.Unpack(value))
    return e;
  if (v.commits < 0)  // deleted entry
    return e;
  if (v.tick < present_tick)
    v.dee = algo::formula_d(0, (double)present_tick, v.dee, (double)v.tick);
  // create!
  e = make_shared<DictEntry>();
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
  if (!ticket.schema) return NULL;
  Config *config = ticket.schema->config();
  bool enable_user_dict = true;
  config->GetBool(ticket.name_space + "/enable_user_dict", &enable_user_dict);
  if (!enable_user_dict)
    return NULL;
  std::string dict_name;
  if (config->GetString(ticket.name_space + "/user_dict", &dict_name)) {
    // user specified name
  }
  else if (config->GetString(ticket.name_space + "/dictionary", &dict_name)) {
    // {dictionary: lunapinyin.extra} implies {user_dict: luna_pinyin}
    size_t dot = dict_name.find('.');
    if (dot != std::string::npos && dot != 0)
      dict_name.resize(dot);
  }
  else {
    LOG(ERROR) << ticket.name_space << "/dictionary not specified in schema '"
               << ticket.schema->schema_id() << "'.";
    return NULL;
  }
  std::string db_class("userdb");
  if (config->GetString(ticket.name_space + "/db_class", &db_class)) {
    // user specified db class
  }
  // obtain userdb object
  shared_ptr<Db> db(db_pool_[dict_name].lock());
  if (!db) {
    Db::Component* c = Db::Require(db_class);
    if (!c) {
      LOG(ERROR) << "undefined db class '" << db_class << "'.";
      return NULL;
    }
    db.reset(c->Create(dict_name));
    db_pool_[dict_name] = db;
  }
  return new UserDictionary(db);
}

}  // namespace rime
