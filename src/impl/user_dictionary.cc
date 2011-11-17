// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/impl/algo.h>
#include <rime/impl/syllablizer.h>
#include <rime/impl/table.h>
#include <rime/impl/user_db.h>
#include <rime/impl/user_dictionary.h>

namespace rime {

static bool unpack_user_dict_value(const std::string &value,
                                   int *commit_count,
                                   double *weight,
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
      else if (k == "w") {
        *weight = boost::lexical_cast<double>(v);
      }
      else if (k == "t") {
        *tick = boost::lexical_cast<TickCount>(v);
      }
    }
    catch (...) {
      EZLOGGERPRINT("key-value parsing failure: '%s'.", k_eq_v.c_str());
      return false;
    }
  }
  return true;
}

struct DfsState {
  TickCount present_tick;
  Code code;
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
  void SaveEntry(int pos);
  bool NextEntry() {
    if (!accessor->GetNextRecord(&key, &value)) {
      key.clear();
      value.clear();
      return false;  // reached the end
    }
    return true;
  }
  bool ForwardScan(const std::string &prefix) {
    if (!accessor->Forward(prefix))
      return false;
    return NextEntry();
  }
};

void DfsState::SaveEntry(int pos) {
  size_t seperator_pos = key.find('\t');
  if (seperator_pos == std::string::npos)
    return;
  shared_ptr<DictEntry> e(new DictEntry);
  e->text = key.substr(seperator_pos + 1);
  int commit_count = 0;
  double last_weight = 0.0;
  TickCount last_tick = 0;
  if (!unpack_user_dict_value(value, &commit_count, &last_weight, &last_tick))
    return;
  e->commit_count = commit_count;
  e->weight = algo::formula_d(0, (double)present_tick, last_weight, (double)last_tick);
  e->code = code;
  EZLOGGERPRINT("pos = %d, text = '%s', code_len = %d, present_tick = %llu, weight = %f, commit_count = %d",
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

bool UserDictionary::DfsLookup(const SyllableGraph &syll_graph, size_t current_pos,
                               const std::string &current_prefix,
                               DfsState *state) {
  EdgeMap::const_iterator edges = syll_graph.edges.find(current_pos);
  if (edges == syll_graph.edges.end()) {
    return true;  // continue DFS lookup
  }
  std::string prefix;
  BOOST_FOREACH(const EndVertexMap::value_type &edge, edges->second) {
    int end_vertex_pos = edge.first;
    const SpellingMap &spellings(edge.second);
    BOOST_FOREACH(const SpellingMap::value_type &spelling, spellings) {
      SyllableId syll_id = spelling.first;
      state->code.push_back(syll_id);
      if (!TranslateCodeToString(state->code, &prefix))
        continue;
      if (prefix > state->key) {  // 'a b c |d ' > 'a b c \tabracadabra'
        if (!state->ForwardScan(prefix)) {
          return false;  // terminate DFS lookup
        }
      }
      while (state->IsExactMatch(prefix)) {  // 'b |e ' vs. 'b e \tBe'
        EZLOGGERVAR(prefix);
        state->SaveEntry(end_vertex_pos);
        if (!state->NextEntry())
          return false;
      }
      if (state->IsPrefixMatch(prefix)) {  // 'b |e ' vs. 'b e f \tBefore'
        if (!DfsLookup(syll_graph, end_vertex_pos, prefix, state))
          return false;
      }
      state->code.pop_back();
      if (!state->IsPrefixMatch(current_prefix)) {  // 'b |' vs. 'g o \tGo'
        return true;  // pruning: done with the current prefix code
      }
      // 'b |e ' vs. 'b y \tBy'
    }
  }
  return true;  // finished traversing the syllable graph
}

shared_ptr<UserDictEntryCollector> UserDictionary::Lookup(const SyllableGraph &syll_graph, size_t start_pos) {
  if (!table_ || !prism_ || !loaded() ||
      start_pos >= syll_graph.interpreted_length)
    return shared_ptr<UserDictEntryCollector>();
  FetchTickCount();
  DfsState state;
  state.present_tick = tick_ + 1;
  state.collector.reset(new UserDictEntryCollector);
  state.accessor.reset(new UserDbAccessor(db_->Query("")));
  state.accessor->Forward(" ");  // skip metadata
  std::string prefix;
  DfsLookup(syll_graph, start_pos, prefix, &state);
  if (state.collector->empty())
    return shared_ptr<UserDictEntryCollector>();
  // sort each group of homophones by weight ASC (to later retrieve with pop_back())
  BOOST_FOREACH(UserDictEntryCollector::value_type &v, *state.collector) {
    v.second.Sort();
  }
  return state.collector;
}

bool UserDictionary::UpdateEntry(const DictEntry &entry, int commit) {
  std::string code_str;
  if (!TranslateCodeToString(entry.code, &code_str))
    return false;
  if (commit == 0)
    return true;
  std::string key(code_str + '\t' + entry.text);
  std::string value;
  int commit_count = 0;
  double weight = 0.0;
  TickCount last_tick = 0;
  if (db_->Fetch(key, &value)) {
    unpack_user_dict_value(value, &commit_count, &weight, &last_tick);
  }
  if (commit > 0) {
    commit_count += commit;
  }
  else if (commit < 0) {  // mark as deleted
    commit_count = (std::min)(-1, -commit_count);
  }
  UpdateTickCount(1);
  weight = algo::formula_d(commit, (double)tick_, weight, (double)last_tick);
  value = boost::str(boost::format("c=%1% w=%2% t=%3%") % 
                     commit_count % weight % tick_);
  EZLOGGERPRINT("Updating entry: %s = '%s'", key.c_str(), value.c_str());
  return db_->Update(key, value);
}

bool UserDictionary::UpdateTickCount(TickCount increment) {
  tick_ += increment;
  try {
    return db_->Update("\0x01/tick", boost::lexical_cast<std::string>(tick_));
  }
  catch (...) {
    return false;
  }
}

bool UserDictionary::Initialize() {
  // TODO: something else?
  return db_->Update("\0x01/tick", "0");
}

bool UserDictionary::FetchTickCount() {
  std::string value;
  try {
    if (!db_->Fetch("\0x01/tick", &value))
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
  shared_ptr<UserDb> user_db(user_db_pool_[dict_name].lock());
  if (!user_db) {
    user_db.reset(new UserDb(dict_name));
    user_db_pool_[dict_name] = user_db;
  }
  return new UserDictionary(user_db);
}

}  // namespace rime
