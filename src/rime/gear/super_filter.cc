// librime/src/rime/gear/super_filter.cc
#include <rime/gear/super_filter.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/dict/db.h>
#include <rime_api.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include <mutex>

namespace rime {

// Process-level global cache for the LevelDb instance.
// Prevents high I/O latency and stuttering when Rime frequently recreates sessions (e.g., switching windows).
struct SuperDbCache {
    an<Db> db;
    std::string db_name;
    std::string files_sig;
};

static SuperDbCache& GetGlobalDbCache() {
    static SuperDbCache cache;
    return cache;
}

static std::vector<size_t> GetUtf8Offsets(const std::string& text) {
    std::vector<size_t> offsets;
    size_t i = 0;
    while (i < text.length()) {
        offsets.push_back(i);
        unsigned char c = text[i];
        if (c < 0x80) i += 1;
        else if (c < 0xE0) i += 2;
        else if (c < 0xF0) i += 3;
        else i += 4;
    }
    offsets.push_back(text.length());
    return offsets;
}

static std::vector<std::string> Split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    if (str.empty()) return tokens; 
    if (delim.empty()) {
        tokens.push_back(str);
        return tokens;
    }
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos <= str.length() && prev < str.length());
    return tokens;
}

SuperFilterTranslation::SuperFilterTranslation(
    an<Translation> inner,
    const std::vector<SuperRule>& rules,
    an<Db> db,
    Context* ctx,
    const std::string& delimiter,
    const std::string& comment_format,
    bool is_chain)
    : inner_(inner), rules_(rules), db_(db), ctx_(ctx),
      delimiter_(delimiter), comment_format_(comment_format), is_chain_(is_chain) {

    size_t start = 0;
    size_t end = ctx_->input().length();
    if (inner_ && !inner_->exhausted()) {
        auto first_cand = inner_->Peek();
        if (first_cand) {
            start = first_cand->start();
            end = first_cand->end();
        }
    }

    // Pre-calculate abbreviation matches upon input change.
    std::string seg_input = ctx_->input().substr(start, end - start);
    GenerateAbbrevCandidates(seg_input, start, end);
    UpdateExhausted();
}

void SuperFilterTranslation::UpdateExhausted() {
    // Lazy evaluation: fetch only the required amount of candidates from the inner translation.
    while (pending_candidates_.empty() && !inner_->exhausted()) {
        ProcessNextInner();
    }
    set_exhausted(index_cands_.empty() && quality_cands_.empty() &&
                  pending_candidates_.empty() && lazy_cands_.empty() && inner_->exhausted());
}

an<Candidate> SuperFilterTranslation::Peek() {
    if (exhausted()) return nullptr;

    // Dispatch priority:
    // 1. Exact index matches (forced insertion)
    if (!index_cands_.empty() && (yield_count_ + 1) >= index_cands_.front().value) {
        return index_cands_.front().cand;
    }

    // 2. Quality threshold matches (dynamic insertion)
    if (!quality_cands_.empty()) {
        if (pending_candidates_.empty() || pending_candidates_.front()->quality() < quality_cands_.front().value) {
            return quality_cands_.front().cand;
        }
    }

    // 3. Regular pipeline candidates
    if (!pending_candidates_.empty()) {
        return pending_candidates_.front();
    }

    // 4. Flush remaining priority queues if the main pipeline is exhausted
    if (!index_cands_.empty()) return index_cands_.front().cand;
    if (!quality_cands_.empty()) return quality_cands_.front().cand;

    // 5. Fallback candidates (quality=0)
    if (!lazy_cands_.empty()) {
        return lazy_cands_.front();
    }

    return nullptr;
}

bool SuperFilterTranslation::Next() {
    if (exhausted()) return false;

    an<Candidate> p = Peek();
    if (!index_cands_.empty() && p == index_cands_.front().cand) index_cands_.pop_front();
    else if (!quality_cands_.empty() && p == quality_cands_.front().cand) quality_cands_.pop_front();
    else if (!pending_candidates_.empty() && p == pending_candidates_.front()) pending_candidates_.pop_front();
    else if (!lazy_cands_.empty() && p == lazy_cands_.front()) lazy_cands_.pop_front();

    yield_count_++;
    UpdateExhausted();
    return !exhausted();
}

// Forward Maximum Matching algorithm for segmenting and replacing long phrases.
std::string SuperFilterTranslation::SegmentConvert(const std::string& text, const std::string& prefix, bool sentence) {
    if (!db_) return text;

    if (!sentence) {
        std::string val;
        if (db_->Fetch(prefix + text, &val)) {
            auto parts = Split(val, delimiter_);
            return parts.empty() ? text : parts[0];
        }
        return text;
    }

    std::vector<size_t> offsets = GetUtf8Offsets(text);
    if (offsets.size() <= 1) return text;
    size_t char_count = offsets.size() - 1;
    std::string result;
    size_t i = 0;
    const size_t MAX_LOOKAHEAD = 6;

    while (i < char_count) {
        bool matched = false;
        size_t max_j = std::min(i + MAX_LOOKAHEAD, char_count);
        for (size_t j = max_j; j > i; --j) {
            std::string sub_text = text.substr(offsets[i], offsets[j] - offsets[i]);
            std::string val;
            if (db_->Fetch(prefix + sub_text, &val)) {
                auto parts = Split(val, delimiter_);
                result += parts.empty() ? sub_text : parts[0];
                i = j;
                matched = true;
                break;
            }
        }
        if (!matched) {
            std::string single = text.substr(offsets[i], offsets[i+1] - offsets[i]);
            std::string val;
            if (db_->Fetch(prefix + single, &val)) {
                auto parts = Split(val, delimiter_);
                result += parts.empty() ? single : parts[0];
            } else {
                result += single;
            }
            i++;
        }
    }
    return result;
}

void SuperFilterTranslation::GenerateAbbrevCandidates(const std::string& input_code, size_t start, size_t end) {
    if (!db_) return;
    abbrev_yielded_.clear();
    for (const auto& r : rules_) {
        if (r.mode == "abbrev") {
            bool is_active = r.always_on;
            if (!is_active) {
                // Dynamically check Rime options state
                for (const auto& opt : r.options) {
                    if (ctx_->get_option(opt)) { is_active = true; break; }
                }
            }
            if (!is_active) continue;

            std::string val;
            if (!db_->Fetch(r.prefix + input_code, &val)) {
                std::string upper_code = input_code;
                for (auto& c : upper_code) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                }
                db_->Fetch(r.prefix + upper_code, &val);
            }

            if (!val.empty()) {
                auto parts = Split(val, delimiter_);
                int count = 0;
                for (const auto& p : parts) {
                    std::string item_text = p;
                    std::string item_preedit = input_code;

                    if (abbrev_yielded_.count(item_text)) continue;
                    abbrev_yielded_.insert(item_text);
                    count++;

                    auto cand = New<SimpleCandidate>(r.cand_type, start, end, item_text, "");
                    cand->set_preedit(item_preedit);

                    if (count <= r.always_qty) {
                        if (r.order_type == "index") {
                            cand->set_quality(999);
                            index_cands_.push_back({cand, r.order_value + (count - 1)});
                        } else if (r.order_type == "quality") {
                            cand->set_quality(r.order_value);
                            quality_cands_.push_back({cand, r.order_value});
                        }
                    } else {
                        // Fallback candidates sink to the bottom with quality 0
                        cand->set_quality(0);
                        lazy_cands_.push_back(cand);
                    }
                }
            }
        }
    }

    std::sort(index_cands_.begin(), index_cands_.end(), [](const InjectCand& a, const InjectCand& b) {
        return a.value < b.value;
    });
    std::sort(quality_cands_.begin(), quality_cands_.end(), [](const InjectCand& a, const InjectCand& b) {
        return a.value > b.value;
    });
}

struct CandData {
    std::string text;
    std::string comment;
    std::string cand_type;
    bool is_original;
};

void SuperFilterTranslation::ProcessNextInner() {
    if (inner_->exhausted()) return;
    auto cand = inner_->Peek();
    inner_->Next();

    if (!cand) return;

    std::vector<CandData> current_items;
    current_items.push_back({cand->text(), cand->comment(), cand->type(), true});

    if (db_) {
        for (const auto& r : rules_) {
            if (r.mode == "abbrev") continue;

            bool is_active = r.always_on;
            if (!is_active) {
                for (const auto& opt : r.options) {
                    if (ctx_->get_option(opt)) { is_active = true; break; }
                }
            }
            if (!is_active) continue;

            std::vector<CandData> next_items;

            for (const auto& item : current_items) {
                std::string val;
                if (r.sentence) {
                    std::string fmm_res = SegmentConvert(item.text, r.prefix, true);
                    if (fmm_res != item.text) val = fmm_res;
                } else {
                    db_->Fetch(r.prefix + item.text, &val);
                }

                if (!val.empty()) {
                    auto parts = Split(val, delimiter_);
                    if (r.t9_mode) {
                        for (auto& p : parts) {
                            size_t delim_pos = p.find("==");
                            if (delim_pos != std::string::npos) {
                                p = p.substr(0, delim_pos);
                            }
                        }
                    }
                    std::string rule_comment = "";
                    if (r.comment_mode == "text" && !item.text.empty()) {
                        std::string cfmt = comment_format_;
                        size_t pos = cfmt.find("%s");
                        if (pos != std::string::npos) {
                            cfmt.replace(pos, 2, item.text);
                            rule_comment = cfmt;
                        } else {
                            rule_comment = item.text;
                        }
                    } else if (r.comment_mode == "append") {
                        rule_comment = item.comment;
                    }

                    if (r.mode == "replace") {
                        for (size_t i = 0; i < parts.size(); ++i) {
                            std::string final_comment = (i == 0 && r.comment_mode == "none") ? "" : rule_comment;
                            std::string ctype = (i == 0 && item.is_original) ? item.cand_type : r.cand_type;
                            next_items.push_back({parts[i], final_comment, ctype, false});
                        }
                    } else if (r.mode == "append") {
                        next_items.push_back(item);
                        for (const auto& p : parts) {
                            std::string final_comment = (r.comment_mode == "none") ? "" : rule_comment;
                            next_items.push_back({p, final_comment, r.cand_type, false});
                        }
                    } else if (r.mode == "comment") {
                        std::string joined;
                        for(size_t i=0; i<parts.size(); ++i) { joined += parts[i] + (i<parts.size()-1?" ":""); }
                        
                        std::string cfmt = comment_format_;
                        size_t pos = cfmt.find("%s");
                        if (pos != std::string::npos) {
                            cfmt.replace(pos, 2, joined);
                        } else {
                            cfmt = joined;
                        }
                        std::string new_comment;
                        if (r.comment_mode == "none") {
                            new_comment = ""; 
                        } else if (r.comment_mode == "text") {
                            new_comment = cfmt; 
                        } else {
                            new_comment = item.comment + cfmt; 
                        }
                        
                        next_items.push_back({item.text, new_comment, item.cand_type, item.is_original});
                    }
                } else {
                    next_items.push_back(item);
                }
            }

            // Pipeline flow control: Hand over the payload to the next rule if chain mode is enabled.
            if (is_chain_) {
                current_items = std::move(next_items);
            } else {
                std::vector<CandData> parallel_merged;
                for (const auto& og : current_items) parallel_merged.push_back(og);
                for (const auto& nx : next_items) {
                    if (!nx.is_original) parallel_merged.push_back(nx);
                }
                current_items = std::move(parallel_merged);
            }
        }
    }

    for (const auto& result : current_items) {
        auto nc = New<SimpleCandidate>(result.cand_type, cand->start(), cand->end(), result.text, result.comment);
        nc->set_quality(cand->quality());
        nc->set_preedit(cand->preedit());
        pending_candidates_.push_back(nc);
    }
}

SuperFilter::SuperFilter(const Ticket& ticket) : Filter(ticket) {
    if (ticket.schema) {
        LoadConfig(ticket.schema->config());
        InitializeDb();
    }
}

SuperFilter::~SuperFilter() {
    // The globally cached LevelDb instance remains open across session lifetimes.
}

void SuperFilter::LoadConfig(Config* config) {
    config->GetString("super_filter/db_name", &db_name_);

    if (!db_name_.empty()) {
        db_name_ = std::filesystem::path(db_name_).filename().string();
    }

    if (db_name_.empty() || db_name_ == "." || db_name_ == "..") {
        db_name_ = "super_filter";
    }
    db_name_ = "data/" + db_name_;

    config->GetString("super_filter/delimiter", &delimiter_);
    if (delimiter_.empty()) delimiter_ = "|";
    config->GetString("super_filter/comment_format", &comment_format_);
    if (comment_format_.empty()) comment_format_ = "〔%s〕";
    config->GetBool("super_filter/chain", &chain_);

    auto root = config->GetItem("super_filter/rules");
    if (auto rule_list = As<ConfigList>(root)) {
        for (size_t i = 0; i < rule_list->size(); ++i) {
            auto item = As<ConfigMap>(rule_list->GetAt(i));
            if (!item) continue;

            SuperRule rule;

            if (auto name_val = As<ConfigValue>(item->Get("name"))) {
                rule.name = name_val->str();
            } else {
                rule.name = "Rule_" + std::to_string(i + 1);
            }

            auto opt_node = item->Get("option");
            if (auto opt_val = As<ConfigValue>(opt_node)) {
                if (opt_val->str() == "true") {
                    rule.always_on = true;
                } else if (opt_val->str() == "false") {
                    // Explicitly frozen rule, option vector remains empty.
                } else {
                    rule.options.push_back(opt_val->str());
                }
            } else if (auto opt_list = As<ConfigList>(opt_node)) {
                for (size_t j=0; j<opt_list->size(); ++j) {
                    if (auto v = As<ConfigValue>(opt_list->GetAt(j))) rule.options.push_back(v->str());
                }
            }

            // Discard disabled or misconfigured rules during the parse phase to save CPU cycles.
            if (!rule.always_on && rule.options.empty()) {
                LOG(INFO) << "super_filter: [" << rule.name << "] frozen or missing option, safely ignored.";
                continue;
            }

            if (auto mode_val = As<ConfigValue>(item->Get("mode"))) rule.mode = mode_val->str();
            else rule.mode = "append";

            if (rule.mode != "append" && rule.mode != "replace" && rule.mode != "comment" && rule.mode != "abbrev") {
                LOG(WARNING) << "super_filter: [" << rule.name << "] unsupported mode '" << rule.mode << "', skipping.";
                continue;
            }

            if (auto sent_val = As<ConfigValue>(item->Get("sentence"))) {
                if (sent_val->str() == "true") rule.sentence = true;
            }

            if (auto pre_val = As<ConfigValue>(item->Get("prefix"))) {
                rule.prefix = pre_val->str();
            } else {
                rule.prefix = "";
            }

            if (auto cmod_val = As<ConfigValue>(item->Get("comment_mode"))) rule.comment_mode = cmod_val->str();
            else rule.comment_mode = "none";

            if (auto ctype_val = As<ConfigValue>(item->Get("cand_type"))) rule.cand_type = ctype_val->str();
            else rule.cand_type = "derived";

            // 解析 T9 模式开关，并设立严格防火墙：仅允许 abbrev 模式使用
            if (auto t9_val = As<ConfigValue>(item->Get("t9_mode"))) {
                rule.t9_mode = (t9_val->str() == "true");
                if (rule.t9_mode && rule.mode != "abbrev") {
                    LOG(WARNING) << "super_filter: [" << rule.name << "] t9_mode 仅支持在 abbrev 模式下开启，已自动忽略。";
                    rule.t9_mode = false;
                }
            }

            if (rule.mode == "abbrev") {
                auto ord_val = As<ConfigValue>(item->Get("order"));
                if (!ord_val) {
                    LOG(WARNING) << "super_filter: [" << rule.name << "] missing 'order' parameter in abbrev mode.";
                    continue; 
                }
                auto parts = Split(ord_val->str(), ",");
                if (parts.size() < 2) {
                    LOG(WARNING) << "super_filter: [" << rule.name << "] malformed 'order' format.";
                    continue;
                }
                try {
                    rule.order_type = parts[0];
                    rule.order_value = std::stoi(parts[1]);
                    if (parts.size() >= 3) rule.always_qty = std::stoi(parts[2]);
                } catch (...) {
                    LOG(WARNING) << "super_filter: [" << rule.name << "] parse exception in 'order'.";
                    continue; 
                }
            } 

            auto files_node = item->Get("files");
            if (!files_node || (!As<ConfigList>(files_node) && !As<ConfigValue>(files_node))) {
                LOG(WARNING) << "super_filter: [" << rule.name << "] missing 'files' dependency.";
                continue;
            }

            if (auto files_list = As<ConfigList>(item->Get("files"))) {
                for(size_t j=0; j<files_list->size(); ++j) {
                    if (auto f = As<ConfigValue>(files_list->GetAt(j))) {
                        std::string filepath = f->str();
                        if (!filepath.empty() && 
                            filepath.front() != '/' && filepath.front() != '\\' && 
                            filepath.find("..") == std::string::npos) {
                            rule.files.push_back(filepath);
                        } else {
                            LOG(WARNING) << "super_filter: [" << rule.name << "] 非法或不安全的词库路径被拦截: " << filepath;
                        }
                    }
                }
            }
            if (rule.files.empty()) {
                LOG(WARNING) << "super_filter: [" << rule.name << "] 过滤后无可用的合法 files，已跳过装载！";
                continue;
            }
            
            rules_.push_back(rule);
        }
    }
}

// Generates a stringent signature combining prefixes, file paths, and system attributes
// to accurately trigger database rebuilds only when necessary.
std::string SuperFilter::GenerateFilesSignature() {
    std::string sig = "delim:" + delimiter_ + "||";
    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    std::error_code ec_exist;

    for (const auto& rule : rules_) {
        sig += "t9:" + std::to_string(rule.t9_mode) + "@";
        for (const auto& path : rule.files) {
            sig += "prefix:" + rule.prefix + "@path:" + path + "=";
            std::filesystem::path full_path = user_dir + "/" + path;
            
            if (std::filesystem::exists(full_path, ec_exist) && !ec_exist) {
                std::error_code ec_time;
                auto ftime = std::filesystem::last_write_time(full_path, ec_time);
                
                std::error_code ec_size;
                auto fsize = std::filesystem::file_size(full_path, ec_size);
                
                if (!ec_time && !ec_size) {
                    auto time_sec = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
                    sig += std::to_string(fsize) + "_" + std::to_string(time_sec) + "|";
                }
            }
        }
    }
    return sig;
}
static std::mutex g_db_cache_mutex;
void SuperFilter::InitializeDb() {
    std::lock_guard<std::mutex> lock(g_db_cache_mutex);
    auto& cache = GetGlobalDbCache();
    std::string current_sig = GenerateFilesSignature();

    // Cache Hit: Instantly mount the pre-opened LevelDb to eliminate I/O lag.
    if (cache.db && cache.db_name == db_name_ && cache.files_sig == current_sig) {
        db_ = cache.db;
        return;
    }

    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    std::error_code ec_dir;
    std::filesystem::create_directories(user_dir + "/data", ec_dir);
    if (ec_dir) {
        LOG(ERROR) << "super_filter: 无法创建 data 文件夹 '" << (user_dir + "/data") 
                   << "': " << ec_dir.message();
        return;
    }

    auto* db_component = Db::Require("userdb");
    if (!db_component) return;

    an<Db> new_db = an<Db>(db_component->Create(db_name_));
    if (!new_db) return;

    bool need_rebuild = false;

    if (new_db->OpenReadOnly()) {
        std::string db_sig;
        new_db->MetaFetch("_files_sig", &db_sig);
        if (db_sig != current_sig) need_rebuild = true;
        new_db->Close();
    } else {
        need_rebuild = true;
    }

    if (need_rebuild) {
        if (new_db->Open()) {
            LOG(INFO) << "super_filter: Database schema updated, initiating LevelDb rebuild...";
            db_ = new_db;
            RebuildDb();
            new_db->MetaUpdate("_files_sig", current_sig);
            LOG(INFO) << "super_filter: LevelDb rebuild complete.";
            new_db->Close();
            db_.reset();
        }
    }

    if (new_db->OpenReadOnly()) {
        cache.db = new_db;
        cache.db_name = db_name_;
        cache.files_sig = current_sig;
        db_ = new_db;
    }
}

// Data structure for in-memory sorting before writing to LevelDb
struct DictItem {
    std::string value;
    double weight;
    int order;
};

void SuperFilter::RebuildDb() {
    if (db_) {
        auto accessor = db_->Query("");
        if (accessor) {
            std::string key, value;
            while (!accessor->exhausted()) {
                if (accessor->GetNextRecord(&key, &value)) {
                    db_->Erase(key);
                }
            }
        }
    }
    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    for (const auto& rule : rules_) {
        // Build a temporary in-memory map to aggregate keys across multiple lines/files
        std::unordered_map<std::string, std::vector<DictItem>> merged_data;
        int line_counter = 0;

        for (const auto& path : rule.files) {
            std::string full_path = user_dir + "/" + path;
            std::ifstream file(full_path);
            if (!file.is_open()) {
                LOG(WARNING) << "super_filter: 无法打开词库文件 (被占用或不存在): " << full_path;
                continue;
            }
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;

                size_t sep1 = line.find_first_of(" \t");
                if (sep1 != std::string::npos) {
                    std::string key = line.substr(0, sep1);
                    std::string orig_key = key;

                    static const char t9_map[26] = {
                        '2','2','2', '3','3','3', '4','4','4', '5','5','5', '6','6','6', 
                        '7','7','7','7', '8','8','8', '9','9','9','9'
                    };

                    if (rule.t9_mode) {
                        for (char& c : key) {
                            if (c >= 'a' && c <= 'z') c = t9_map[c - 'a'];
                            else if (c >= 'A' && c <= 'Z') c = t9_map[c - 'A'];
                        }
                    }

                    size_t val_start = line.find_first_not_of(" \t", sep1);
                    if (val_start != std::string::npos) {
                        std::string rest = line.substr(val_start);
                        rest.erase(rest.find_last_not_of("\r\n \t") + 1); // trim right

                        std::string val = rest;
                        
                        // 如果是 T9 模式，且值里还没有包含 ==，把原始拼音作为尾巴藏进去
                        if (rule.t9_mode && val.find("==") == std::string::npos) {
                            val = val + "==" + orig_key;
                        }
                        double weight = 0.0;

                        // Try to extract weight from a potential 3rd column
                        size_t last_delim = rest.find_last_of(" \t");
                        if (last_delim != std::string::npos) {
                            size_t weight_start = rest.find_first_not_of(" \t", last_delim);
                            if (weight_start != std::string::npos) {
                                std::string weight_str = rest.substr(weight_start);
                                try {
                                    size_t parsed_len;
                                    weight = std::stod(weight_str, &parsed_len);
                                    // Ensure the parsed number spans the entire rest of the string
                                    if (parsed_len == weight_str.length()) {
                                        val = rest.substr(0, last_delim);
                                        val.erase(val.find_last_not_of(" \t") + 1);
                                    } else {
                                        weight = 0.0;
                                    }
                                } catch (...) {
                                    weight = 0.0;
                                }
                            }
                        }
                        
                        // Push into the map (grouped by prefix + key)
                        merged_data[rule.prefix + key].push_back({val, weight, line_counter++});
                    }
                }
            }
        }

        // Sort items by weight and merge them into a single string for DB insertion
        for (auto& kv : merged_data) {
            auto& items = kv.second;
            
            // Sort logic: Descending by weight, Ascending by original read order
            std::sort(items.begin(), items.end(), [](const DictItem& a, const DictItem& b) {
                if (a.weight != b.weight) return a.weight > b.weight;
                return a.order < b.order;
            });

            std::string final_val;
            for (size_t i = 0; i < items.size(); ++i) {
                final_val += items[i].value;
                if (i < items.size() - 1) final_val += delimiter_;
            }

            db_->Update(kv.first, final_val);
        }
    }
}

an<Translation> SuperFilter::Apply(
    an<Translation> translation,
    CandidateList* candidates) {

    if (!translation) return nullptr;
    Context* ctx = engine_->context();

    if (!ctx->IsComposing() || ctx->input().empty()) {
        return translation;
    }

    return New<SuperFilterTranslation>(translation, rules_, db_, ctx, delimiter_, comment_format_, chain_);
}

} // namespace rime