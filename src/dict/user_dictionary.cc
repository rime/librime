// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <map>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/algo/dynamics.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/table.h>
#include <rime/dict/user_db.h>
#include <rime/dict/user_dictionary.h>

namespace rime {

struct DfsState {
  size_t depth_limit;
  TickCount present_tick;
  Code code;
  std::vector<double> credibility;
  shared_ptr<UserDictEntryCollector> collector;
  shared_ptr<UserDbAccessor> accessor;
  std::string key;
  std::string value;
  
  bool IsExactMatch(const std::string &prefix) {
    return boost::starts_with(key, prefix + '\t');
  }
  bool IsPrefixMatch(const std::string &prefix) {
    return boost::starts_with(key, prefix);
  }
  void SaveEntry(size_t pos);
  bool NextEntry() {
    if (!accessor->GetNextRecord(&key, &value)) {
      key.clear();
      value.clear();
      return false;  // reached the end
    }
    return true;
  }
  bool ForwardScan(const std::string &prefix) {
    if (!accessor->Forward(prefix)) {
      return false;
    }
    return NextEntry();
  }
  bool Backdate(const std::string &prefix) {
    EZDBGONLYLOGGERVAR(prefix);
    if (prefix.empty() ?
        !accessor->Reset() :
        !accessor->Forward(prefix)) {
      EZLOGGERPRINT("Warning: backdating failed for '%s'.", prefix.c_str());
      return false;
    }
    return NextEntry();
  }
};

void DfsState::SaveEntry(size_t pos) {
  size_t seperator_pos = key.find('\t');
  if (seperator_pos == std::string::npos)
    return;
  shared_ptr<DictEntry> e = make_shared<DictEntry>();
  e->text = key.substr(seperator_pos + 1);
  int commit_count = 0;
  double dee = 0.0;
  TickCount last_tick = 0;
  if (!UserDictionary::UnpackValues(value, &commit_count, &dee, &last_tick))
    return;
  if (commit_count < 0)  // deleted entry
    return;
  dee = algo::formula_d(0, (double)present_tick, dee, (double)last_tick);
  e->commit_count = commit_count;
  // TODO: argument s not defined...
  e->weight = algo::formula_p(0,
                              (double)commit_count / present_tick,
                              (double)present_tick,
                              dee) * credibility.back();
  e->code = code;
  EZDBGONLYLOGGERPRINT("pos = %d, text = '%s', code_len = %d, present_tick = %llu, weight = %f, commit_count = %d",
                       pos, e->text.c_str(), e->code.size(), present_tick, e->weight, e->commit_count);
  (*collector)[pos].push_back(e);
}

// UserDictionary members

UserDictionary::UserDictionary(const shared_ptr<UserDb> &user_db)
    : db_(user_db), tick_(0) {
}

UserDictionary::~UserDictionary() {
}

void UserDictionary::Attach(const shared_ptr<Table> &table, const shared_ptr<Prism> &prism) {
  table_ = table;
  prism_ = prism;
}

bool UserDictionary::Load() {
  if (!db_ || !db_->Open())
    return false;
  if (!FetchTickCount() && !Initialize())
    return false;
  return true;
}

bool UserDictionary::loaded() const {
  return db_ && db_->loaded();
}

// this is a one-pass scan for the user db which supports sequential access in alphabetical order (of syllables).
// each call to DfsLookup() searches matching phrases at a given start position: current_pos.
// there may be multiple edges that starts at current_pos, and ends at different positions after current_pos.
// on each edge, there may be multiple syllables the spelling on the edge maps to.
// in order to enable forward scaning and to avoid backdating, our strategy is:
// sort all those syllables from edges that starts at current_pos, so that the syllables are in the same
// alphabetical order with the user db's.
// this having been done by transposing the syllable graph into SyllableGraph::index.
// however, in the case of 'shsh' which could be the abbreviation of either 'sh(a) sh(i)' or 'sh(a) s(hi) h(ou)',
// we now have to give up the latter path in order to avoid backdating.

void UserDictionary::DfsLookup(const SyllableGraph &syll_graph, size_t current_pos,
                               const std::string &current_prefix,
                               DfsState *state) {
  SpellingIndices::const_iterator index = syll_graph.indices.find(current_pos);
  if (index == syll_graph.indices.end()) {
    return;
  }
  EZDBGONLYLOGGERPRINT("dfs lookup starts from %d.", current_pos);
  std::string prefix;
  BOOST_FOREACH(const SpellingIndex::value_type& spelling, index->second) {
    if (spelling.second.empty()) continue;
    const SpellingProperties* props = spelling.second[0];
    size_t end_pos = props->end_pos;
    EZDBGONLYLOGGERPRINT("prefix: '%s', syll_id: %d, edge: [%d, %d) of %d",
                         current_prefix.c_str(), spelling.first,
                         current_pos, end_pos, spelling.second.size());
    state->code.push_back(spelling.first);
    state->credibility.push_back(state->credibility.back() * props->credibility);
    BOOST_SCOPE_EXIT( (&state) ) {
      state->code.pop_back();
      state->credibility.pop_back();
    } BOOST_SCOPE_EXIT_END
    if (!TranslateCodeToString(state->code, &prefix))
      continue;
    if (prefix > state->key) {  // 'a b c |d ' > 'a b c \tabracadabra'
      EZDBGONLYLOGGERPRINT("forward scanning for '%s'.", prefix.c_str());
      if (!state->ForwardScan(prefix))  // reached the end of db
        return;
    }
    while (state->IsExactMatch(prefix)) {  // 'b |e ' vs. 'b e \tBe'
      EZDBGONLYLOGGERPRINT("match found for '%s'.", prefix.c_str());
      state->SaveEntry(end_pos);
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

shared_ptr<UserDictEntryCollector> UserDictionary::Lookup(const SyllableGraph &syll_graph,
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
  state.accessor->Forward(" ");  // skip metadata
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

bool UserDictionary::UpdateEntry(const DictEntry &entry, int commit) {
  std::string code_str;
  if (!TranslateCodeToString(entry.code, &code_str))
    return false;
  std::string key(code_str + '\t' + entry.text);
  std::string value;
  int commit_count = 0;
  double dee = 0.0;
  TickCount last_tick = 0;
  if (db_->Fetch(key, &value)) {
    UnpackValues(value, &commit_count, &dee, &last_tick);
  }
  if (commit > 0) {
    if (commit_count < 0)
      commit_count = -commit_count;  // revive a deleted item
    commit_count += commit;
    UpdateTickCount(1);
    dee = algo::formula_d(commit, (double)tick_, dee, (double)last_tick);
  }
  else if (commit == 0) {
    const double k = 0.1;
    dee = algo::formula_d(k, (double)tick_, dee, (double)last_tick);
  }
  else if (commit < 0) {  // mark as deleted
    commit_count = (std::min)(-1, -commit_count);
    dee = algo::formula_d(0.0, (double)tick_, dee, (double)last_tick);
  }
  value = boost::str(boost::format("c=%1% d=%2% t=%3%") % 
                     commit_count % dee % tick_);
  return db_->Update(key, value);
}

bool UserDictionary::UpdateTickCount(TickCount increment) {
  tick_ += increment;
  if (tick_ % 50 == 0) {  // backup every 50 commits
    db_->Backup();
  }
  try {
    return db_->Update("\x01/tick", boost::lexical_cast<std::string>(tick_));
  }
  catch (...) {
    return false;
  }
}

bool UserDictionary::Initialize() {
  return db_->Update("\x01/tick", "0");
}

bool UserDictionary::FetchTickCount() {
  std::string value;
  try {
    // an earlier version mistakenly wrote tick count into an empty key
    if (!db_->Fetch("\x01/tick", &value) &&
        !db_->Fetch("", &value))
      return false;
    tick_ = boost::lexical_cast<TickCount>(value);
    return true;
  }
  catch (...) {
    tick_ = 0;
    return false;
  }
}

bool UserDictionary::TranslateCodeToString(const Code &code, std::string* result) {
  if (!table_ || !result) return false;
  result->clear();
  BOOST_FOREACH(const int &syllable_id, code) {
    const char *spelling = table_->GetSyllableById(syllable_id);
    if (!spelling) {
      EZLOGGERPRINT("Error translating syllable_id '%d' to string.", syllable_id);
      result->clear();
      return false;
    }
    *result += spelling;
    *result += ' ';
  }
  return true;
}

bool UserDictionary::UnpackValues(const std::string &value,
                                  int *commit_count,
                                  double *dee,
                                  TickCount *tick) {
  std::vector<std::string> kv;
  boost::split(kv, value, boost::is_any_of(" "));
  BOOST_FOREACH(const std::string &k_eq_v, kv) {
    size_t eq = k_eq_v.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string k(k_eq_v.substr(0, eq));
    const std::string v(k_eq_v.substr(eq + 1));
    try {
      if (k == "c") {
        *commit_count = boost::lexical_cast<int>(v);
      }
      else if (k == "d") {
        *dee = boost::lexical_cast<double>(v);
      }
      else if (k == "t") {
        *tick = boost::lexical_cast<TickCount>(v);
      }
    }
    catch (...) {
      EZLOGGERPRINT("Error: key-value parsing failed: '%s'.", k_eq_v.c_str());
      return false;
    }
  }
  return true;
}

// UserDictionaryComponent members

UserDictionaryComponent::UserDictionaryComponent() {
}

UserDictionary* UserDictionaryComponent::Create(Schema *schema) {
  if (!schema) return NULL;
  Config *config = schema->config();
  bool enable_user_dict = true;
  config->GetBool("translator/enable_user_dict", &enable_user_dict);
  if (!enable_user_dict)
    return NULL;
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified in schema '%s'.",
                  schema->schema_id().c_str());
    return NULL;
  }
  // obtain userdb object
  shared_ptr<UserDb> db(db_pool_[dict_name].lock());
  if (!db) {
    db = boost::make_shared<UserDb>(dict_name);
    db_pool_[dict_name] = db;
  }
  return new UserDictionary(db);
}

}  // namespace rime
