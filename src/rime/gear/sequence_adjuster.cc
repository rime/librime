// src/rime/gear/sequence_adjuster.cc
#include <rime/translation.h> 
#include <rime/context.h>
#include <rime/composition.h> 
#include <rime/segmentation.h> 
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/dict/db.h>
#include <rime/dict/user_db.h>
#include <rime/gear/sequence_adjuster.h> 
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

// 0. DB 单例管理

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
  bool exhausted() { return cursor_ >= candidates_.size(); } 
 private:
  std::vector<an<Candidate>> candidates_;
  size_t cursor_ = 0;
};

// 2. 数据结构与辅助函数
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
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}
// 始终返回当前 Segment 的完整编码
static std::string GetContextCode(Context* ctx) {
  if (!ctx || ctx->composition().empty()) return "";
  const auto& segment = ctx->composition().back();
  size_t start = segment.start;
  size_t end = segment.end;
  
  if (end > ctx->input().length()) end = ctx->input().length();
  if (start >= end) return "";
  
  std::string code = ctx->input().substr(start, end - start);
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

  // 硬编码 DB 名称
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
  
  // 处理按键
  std::string repr = key_event.repr();
  if (repr == "{Sequence_Up}" || key_event == key_up_)    return SaveAdjustment(ctx, 1, false, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Down}" || key_event == key_down_)  return SaveAdjustment(ctx, -1, false, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Pin}" || key_event == key_pin_)   return SaveAdjustment(ctx, 0, true, false) ? kAccepted : kNoop;
  if (repr == "{Sequence_Reset}" || key_event == key_reset_) return SaveAdjustment(ctx, 0, false, true) ? kAccepted : kNoop;
  
  return kNoop;
}

bool SequenceAdjusterProcessor::SaveAdjustment(Context* ctx, int offset, bool is_pin, bool is_reset) {
  // 1. 获取当前选中的候选词
  auto cand = ctx->GetSelectedCandidate();
  if (!cand) return false;
  
  // 2. 获取 ContextCode (例如 "nihk")
  std::string code = GetContextCode(ctx);
  if (code.empty()) return false;
  
  // 生成 Key: nihk \t 拟
  // 这保证了它只在 nihk 上下文中生效，不污染 ni 的上下文
  std::string key = MakeDbKey(code, cand->text());
  
  auto& segment = ctx->composition().back(); 
  int current_idx = static_cast<int>(segment.selected_index);
  int target_idx = 0;
  
  if (is_pin) target_idx = 0;
  else if (is_reset) target_idx = -1;
  else {
    target_idx = current_idx - offset; 
    if (target_idx < 0) target_idx = 0;
  }

  // 如果是普通移动，必须检查“我”和“目标位置的那个词”是不是同一类（编码长度相同）。
  // 防止 ni (2码) 跳到 nihk (4码) 的区域去。
  if (!is_pin && !is_reset && target_idx != current_idx) {
      auto target_cand = segment.GetCandidateAt(target_idx);
      // 如果目标位置有词，且 目标词的end != 我的end => 长度不同，禁止穿越！
      if (target_cand && target_cand->end() != cand->end()) {
          return true; // 拦截按键，不执行任何操作
      }
  }

  // 4. 原地不动检查
  if (target_idx == current_idx && !is_reset) {
      return true; 
  }

  // 5. 更新 DB
  SequenceDbValue val;
  std::string val_str;
  if (user_db_->Fetch(key, &val_str)) val = SequenceDbValue::Unpack(val_str);

  unsigned long now = GetPhysicalTimestamp();
  if (now <= val.stamp) val.stamp = val.stamp + 1;
  else val.stamp = now;
  
  val.position = target_idx;

  if (user_db_->Update(key, val.Pack())) {
    // 刷新并锁定高亮
    // Processor 确认可以移动后，立即更新高亮，让用户感觉“动了”。
    // 随后 Filter 会根据 DB 真正移动词的位置。
    ctx->RefreshNonConfirmedComposition();
    if (!ctx->composition().empty()) {
       auto& seg_ref = ctx->composition().back();
       if (!is_reset && target_idx >= 0) {
          seg_ref.selected_index = static_cast<size_t>(target_idx);
          seg_ref.status = Segment::kGuess;
       }
    }
    return true;
  }
  return false;
}

// 4. Filter 实现
SequenceAdjusterFilter::SequenceAdjusterFilter(const Ticket& ticket) : Filter(ticket) {
  // 直接在构造函数里初始化，不调用 LoadConfig
  Config* config = engine_->schema()->config();
  bool module_enabled = true;
  config->GetBool("sequence_adjuster/enable", &module_enabled);
  if (module_enabled) {
    db_name_ = "sequence";
    if (!db_name_.empty()) user_db_ = GetSharedDb(db_name_);
  }
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
  
  // 使用 ContextCode 作为 Key，实现上下文隔离
  // 输入 nihk 时，只查 nihk 的记录。
  std::string input_code = GetContextCode(ctx);
  if (input_code.empty()) return translation;

  // 1. 读取 DB
  std::map<std::string, SequenceDbValue> adjustments;
  std::string db_prefix = input_code;
  if (db_prefix.empty() || db_prefix.back() != ' ') db_prefix += ' ';
  db_prefix += "\t"; 

  auto accessor = user_db_->Query(db_prefix);
  if (accessor && accessor->Jump(db_prefix)) {
    std::string key, val_str;
    while (accessor->GetNextRecord(&key, &val_str)) {
      if (!boost::starts_with(key, db_prefix)) break;
      if (key.length() > db_prefix.length()) {
        std::string text = key.substr(db_prefix.length());
        SequenceDbValue val = SequenceDbValue::Unpack(val_str);
        if (val.position >= 0 && val.stamp > 0) {
          adjustments[text] = val;
        }
      }
    }
  }

  // 2. 全量读取 + 去重
  auto list = std::vector<an<Candidate>>();
  std::unordered_set<std::string> seen_texts;
  
  while (!translation->exhausted()) {
    auto cand = translation->Peek();
    if (cand && !cand->text().empty()) {
        if (seen_texts.find(cand->text()) == seen_texts.end()) {
            list.push_back(cand);
            seen_texts.insert(cand->text());
        }
    }
    translation->Next();
  }
  
  if (list.size() <= 1) return New<VectorTranslation>(std::move(list));
  if (adjustments.empty()) return New<VectorTranslation>(std::move(list));

  // 3. 收集动作
  struct Action {
    std::string text;
    int target_index;  
    unsigned long stamp;
  };
  std::vector<Action> actions;

  for (const auto& cand : list) {
    auto it = adjustments.find(cand->text());
    if (it != adjustments.end()) {
        actions.push_back({cand->text(), it->second.position, it->second.stamp});
    }
  }

  // 4. 排序并移动
  std::sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
    return a.stamp < b.stamp; 
  });

  for (const auto& action : actions) {
    auto it = std::find_if(list.begin(), list.end(), [&](const an<Candidate>& c) {
      return c->text() == action.text;
    });
    
    if (it != list.end()) {
      int current_idx = std::distance(list.begin(), it);
      int target = action.target_index;
      if (target > (int)list.size()) target = (int)list.size();
      if (target < 0) target = 0;

      if (current_idx == target) continue;

      an<Candidate> cand = *it;
      list.erase(it);
      
      if (target >= list.size()) list.push_back(cand);
      else list.insert(list.begin() + target, cand);
    }
  }

  return New<VectorTranslation>(std::move(list));
}

}  // namespace rime
