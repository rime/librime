#include <rime/context.h>
#include <rime/composition.h> 
#include <rime/segmentation.h> 
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/dict/db.h>
#include <rime/dict/user_db.h>
#include <rime/gear/sequence_adjuster.h> 
#include <rime/translation.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm> 
#include <chrono>
#include <map>
#include <unordered_set>
#include <glog/logging.h>

namespace rime {

// 0. DB 单例
static std::weak_ptr<Db> g_shared_db;
static std::mutex g_db_mutex;

static an<Db> GetSharedDb(const std::string& db_name) {
  std::lock_guard<std::mutex> lock(g_db_mutex);
  an<Db> db = g_shared_db.lock();
  if (db) return db;
  auto component = UserDb::Require("userdb");
  if (!component) return nullptr;
  
  db.reset(component->Create(db_name));
  if (db) {
    if (db->Open()) {
      g_shared_db = db;
    } else {
      db.reset();
    }
  }
  return db;
}

// 1. VectorTranslation

class VectorTranslation : public Translation {
 public:
  VectorTranslation(std::vector<an<Candidate>> candidates)
      : candidates_(std::move(candidates)) {}
  bool Next() override {
    if (exhausted()) return false;
    cursor_++;
    return true;
  }
  an<Candidate> Peek() override {
    if (exhausted()) return nullptr;
    return candidates_[cursor_];
  }
  bool exhausted() const { return cursor_ >= candidates_.size(); }
 private:
  std::vector<an<Candidate>> candidates_;
  size_t cursor_ = 0;
};

// 2. SequenceDbValue 实现
std::string SequenceDbValue::Pack() const {
  std::ostringstream packed;
  packed << "p=" << position << " s=" << stamp;
  return packed.str();
}

SequenceDbValue SequenceDbValue::Unpack(const std::string& value) {
  SequenceDbValue v;
  std::vector<std::string> kv;
  boost::split(kv, value, boost::is_any_of(" "));
  for (const std::string& k_eq_v : kv) {
    size_t eq = k_eq_v.find('=');
    if (eq == std::string::npos) continue;
    std::string k = k_eq_v.substr(0, eq);
    std::string val = k_eq_v.substr(eq + 1);
    try {
      if (k == "p") v.position = std::stoi(val);
      else if (k == "s") v.stamp = std::stoul(val);
    } catch (...) {}
  }
  return v;
}

static unsigned long GetPhysicalTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}

// GetActiveCode: Input + Caret 逻辑
static std::string GetActiveCode(Context* ctx) {
  if (!ctx) return "";
  
  size_t caret = ctx->caret_pos();
  std::string full_input = ctx->input();
  
  if (caret > full_input.length()) {
    caret = full_input.length();
  }

  std::string code = full_input.substr(0, caret);
  boost::trim(code);
  return code;
}


// 3. Processor 实现
SequenceAdjusterProcessor::SequenceAdjusterProcessor(const Ticket& ticket)
    : Processor(ticket) {
  LoadConfig();
}
SequenceAdjusterProcessor::~SequenceAdjusterProcessor() { user_db_.reset(); }

void SequenceAdjusterProcessor::LoadConfig() {
  Config* config = engine_->schema()->config();
  bool module_enabled = true;
  config->GetBool("sequence_adjuster/enable", &module_enabled);
  if (!module_enabled) return;

  db_name_ = "sequence";
  
  auto load_key = [&](const std::string& key_name, KeyEvent& key, const std::string& def) {
    std::string str;
    if (config->GetString("sequence_adjuster/" + key_name, &str)) key = KeyEvent(str);
    else key = KeyEvent(def);
  };
  
  load_key("up", key_up_, "Control+Shift+J");
  load_key("down", key_down_, "Control+Shift+K");
  load_key("reset", key_reset_, "Control+Shift+L");
  load_key("pin", key_pin_, "Control+Shift+P");
  
  if (!db_name_.empty()) user_db_ = GetSharedDb(db_name_);
}

std::string SequenceAdjusterProcessor::MakeDbKey(const std::string& code, const std::string& text) {
  std::string effective_code = code;
  if (effective_code.empty() || effective_code.back() != ' ') effective_code += ' ';
  return effective_code + "\t" + text;
}

ProcessResult SequenceAdjusterProcessor::ProcessKeyEvent(const KeyEvent& key_event) {
  if (!user_db_) return kNoop;
  Context* ctx = engine_->context();
  if (!ctx->HasMenu()) return kNoop;
  if (key_event.release()) return kNoop;
  
  std::string repr = key_event.repr();
  if (repr == "{Sequence_Up}" || key_event == key_up_)    return SaveAdjustment(ctx, 1, false, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Down}" || key_event == key_down_)  return SaveAdjustment(ctx, -1, false, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Pin}" || key_event == key_pin_)   return SaveAdjustment(ctx, 0, true, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Reset}" || key_event == key_reset_) return SaveAdjustment(ctx, 0, false, true) ? kAccepted : kNoop;
  
  return kNoop;
}

bool SequenceAdjusterProcessor::SaveAdjustment(Context* ctx, int offset, bool is_pin, bool is_reset) {
  auto cand = ctx->GetSelectedCandidate();
  if (!cand) return false;
  
  std::string code = GetActiveCode(ctx);
  if (code.empty()) return false;
  
  std::string key = MakeDbKey(code, cand->text());
  
  int current_idx = 0;
  if (!ctx->composition().empty()) {
      current_idx = static_cast<int>(ctx->composition().back().selected_index);
  }

  int target_idx = 0;
  if (is_pin) target_idx = 0;
  else if (is_reset) target_idx = -1;
  else {
    target_idx = current_idx - offset; 
    if (target_idx < 0) target_idx = 0;
  }

  SequenceDbValue val;
  std::string val_str;
  if (user_db_->Fetch(key, &val_str)) val = SequenceDbValue::Unpack(val_str);

  unsigned long now = GetPhysicalTimestamp();
  if (now <= val.stamp) val.stamp = val.stamp + 1;
  else val.stamp = now;
  
  val.position = target_idx;

  if (user_db_->Update(key, val.Pack())) {
    ctx->RefreshNonConfirmedComposition();
    if (!is_reset && target_idx >= 0 && !ctx->composition().empty()) {
      auto& segment = ctx->composition().back();
      segment.selected_index = static_cast<size_t>(target_idx);
      segment.status = Segment::kGuess; 
    }
    return true;
  }
  return false;
}

// 4. Filter 实现
SequenceAdjusterFilter::SequenceAdjusterFilter(const Ticket& ticket) : Filter(ticket) {
  Config* config = engine_->schema()->config();
  bool module_enabled = true;
  config->GetBool("sequence_adjuster/enable", &module_enabled);
  if (!module_enabled) return;
  
  db_name_ = "sequence";
  if (!db_name_.empty()) user_db_ = GetSharedDb(db_name_);
}
SequenceAdjusterFilter::~SequenceAdjusterFilter() { user_db_.reset(); }

std::string SequenceAdjusterFilter::MakeDbKey(const std::string& code, const std::string& text) {
  std::string effective_code = code;
  if (effective_code.empty() || effective_code.back() != ' ') effective_code += ' ';
  return effective_code + "\t" + text;
}

an<Translation> SequenceAdjusterFilter::Apply(an<Translation> translation, CandidateList* candidates) {
  if (!user_db_ || !translation || translation->exhausted()) return translation;
  
  Context* ctx = engine_->context();
  std::string input_code = GetActiveCode(ctx);
  if (input_code.empty()) return translation;

  std::string db_prefix = input_code;
  if (db_prefix.empty() || db_prefix.back() != ' ') {
    db_prefix += ' ';
  }
  db_prefix += "\t"; 

  // 1. 批量查询 (Batch Query)
  std::map<std::string, SequenceDbValue> adjustments;
  auto accessor = user_db_->Query(db_prefix);
  
  if (accessor && accessor->Jump(db_prefix)) {
    std::string key, val_str;
    while (accessor->GetNextRecord(&key, &val_str)) {
      if (!boost::starts_with(key, db_prefix)) break;

      // 解析 Key: [Code] [\t] [Text]
      // 这里的逻辑兼容了 DB 里的 "ribk\t" 和 "ribk \t"
      if (key.length() > db_prefix.length()) {
        std::string text = key.substr(db_prefix.length());
        SequenceDbValue val = SequenceDbValue::Unpack(val_str);
        if (val.position >= 0 && val.stamp > 0) {
          adjustments[text] = val;
        }
      }
    }
  }

  // 2. 候选词转换 + 去重 (Deduplication)
  // 这里解决了同名候选词导致的移动 BUG
  auto list = std::vector<an<Candidate>>();
  // 使用 unordered_set 记录已存在的文本，实现 O(1) 查找
  std::unordered_set<std::string> seen; 

  while (!translation->exhausted()) {
    auto cand = translation->Peek();
    if (cand) {
        // 如果这个词的文本还没出现过，才加入列表
        if (seen.find(cand->text()) == seen.end()) {
            list.push_back(cand);
            seen.insert(cand->text());
        }
    }
    translation->Next();
  }

  if (list.empty()) return New<VectorTranslation>(list);
  if (adjustments.empty()) return New<VectorTranslation>(list);
  // 3. 排序逻辑 (Sort & Apply)
  struct Action {
    std::string text;
    int target_index;  
    unsigned long stamp;
  };
  std::vector<Action> actions;

  for (const auto& cand : list) {
    auto it = adjustments.find(cand->text());
    if (it != adjustments.end()) {
      const auto& val = it->second;
      actions.push_back({cand->text(), val.position, val.stamp});
    }
  }

  std::sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
    return a.stamp < b.stamp; 
  });

  for (const auto& action : actions) {
    auto it = std::find_if(list.begin(), list.end(), [&](const an<Candidate>& c) {
      return c->text() == action.text;
    });
    
    // 这里的 it 绝对是唯一的，因为我们在上面已经去重了
    if (it != list.end()) {
      auto cand = *it;
      list.erase(it);
      
      int target = action.target_index;
      if (target > (int)list.size()) target = (int)list.size();
      if (target < 0) target = 0;
      
      list.insert(list.begin() + target, cand);
    }
  }
  return New<VectorTranslation>(list);
}

}  // namespace rime