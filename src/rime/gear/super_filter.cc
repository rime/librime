// librime/src/rime/gear/super_filter.cc
#include <rime/gear/super_filter.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/candidate.h>
#include <rime_api.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include <mutex>
#include <cstring>

namespace rime {

// SuperBinaryDb
SuperBinaryDb::SuperBinaryDb(const std::string& bin_path) : bin_path_(bin_path) {}

SuperBinaryDb::~SuperBinaryDb() { Close(); }

void SuperBinaryDb::Close() {
    region_.reset();
    mapping_.reset();
    data_ptr_ = nullptr;
    record_count_ = 0;
    index_ptr_ = nullptr;
}

bool SuperBinaryDb::Open() {
    try {
        mapping_ = std::make_unique<boost::interprocess::file_mapping>(bin_path_.c_str(), boost::interprocess::read_only);
        region_ = std::make_unique<boost::interprocess::mapped_region>(*mapping_, boost::interprocess::read_only);
        data_ptr_ = static_cast<const char*>(region_->get_address());
        data_size_ = region_->get_size();

        if (data_size_ < 12) return false;
        if (std::strncmp(data_ptr_, "SUPR", 4) != 0) return false;

        uint32_t sig_len;
        std::memcpy(&sig_len, data_ptr_ + 8, 4);
        
        uint32_t padded_sig_len = (sig_len + 3) & ~3;
        size_t header_offset = 12 + padded_sig_len;
        
        if (data_size_ < header_offset + 4) return false;
        std::memcpy(&record_count_, data_ptr_ + header_offset, 4);
        
        size_t index_offset = header_offset + 4;
        index_ptr_ = reinterpret_cast<const uint32_t*>(data_ptr_ + index_offset);
        
        return true;
    } catch (...) {
        return false;
    }
}

bool SuperBinaryDb::Fetch(const std::string& key, std::string* value) const {
    if (!data_ptr_ || record_count_ == 0) return false;

    int left = 0;
    int right = static_cast<int>(record_count_) - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        uint32_t offset = index_ptr_[mid];
        if (offset >= data_size_) return false;

        const char* mid_key = data_ptr_ + offset;

        int cmp = std::strcmp(mid_key, key.c_str());
        if (cmp == 0) {
            size_t key_len = std::strlen(mid_key);
            uint32_t val_offset = offset + key_len + 1;
            if (val_offset >= data_size_) return false;
            
            *value = std::string(data_ptr_ + val_offset);
            return true;
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return false;
}

bool SuperBinaryDb::CheckSignature(const std::string& expected_sig) const {
    std::ifstream in(bin_path_, std::ios::binary);
    if (!in) return false;
    char magic[4];
    if (!in.read(magic, 4) || std::strncmp(magic, "SUPR", 4) != 0) return false;
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), 4);
    uint32_t sig_len;
    in.read(reinterpret_cast<char*>(&sig_len), 4);
    
    std::string sig(sig_len, '\0');
    if (!in.read(&sig[0], sig_len)) return false;
    
    return sig == expected_sig;
}

bool SuperBinaryDb::Build(const std::string& bin_path, const std::string& signature,
                          const std::map<std::string, std::string>& data) {
    std::ofstream out(bin_path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    out.write("SUPR", 4);
    uint32_t version = 1;
    out.write(reinterpret_cast<char*>(&version), 4);
    
    uint32_t sig_len = signature.length();
    out.write(reinterpret_cast<char*>(&sig_len), 4);
    out.write(signature.data(), sig_len);
    
    uint32_t padded_sig_len = (sig_len + 3) & ~3;
    if (padded_sig_len > sig_len) {
        std::vector<char> padding(padded_sig_len - sig_len, '\0');
        out.write(padding.data(), padding.size());
    }

    uint32_t count = data.size();
    out.write(reinterpret_cast<char*>(&count), 4);

    size_t index_start_pos = out.tellp();
    std::vector<uint32_t> index_buffer(count, 0);
    out.write(reinterpret_cast<char*>(index_buffer.data()), index_buffer.size() * sizeof(uint32_t));

    uint32_t current_offset = out.tellp();
    int i = 0;
    
    for (const auto& kv : data) {
        index_buffer[i] = current_offset;
        out.write(kv.first.c_str(), kv.first.length() + 1);
        current_offset += kv.first.length() + 1;

        out.write(kv.second.c_str(), kv.second.length() + 1);
        current_offset += kv.second.length() + 1;
        i++;
    }

    out.seekp(index_start_pos);
    out.write(reinterpret_cast<char*>(index_buffer.data()), index_buffer.size() * sizeof(uint32_t));
    return true;
}

struct SuperDbCacheItem {
    std::shared_ptr<SuperBinaryDb> db;
    std::string files_sig;
};

static std::unordered_map<std::string, SuperDbCacheItem>& GetGlobalDbCache() {
    static std::unordered_map<std::string, SuperDbCacheItem> cache_map;
    return cache_map;
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
        if (i > text.length()) {
            i = text.length();
        }
    }
    if (offsets.empty() || offsets.back() != text.length()) {
        offsets.push_back(text.length());
    }
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

static std::mutex g_db_cache_mutex;

// SuperFilterTranslation
SuperFilterTranslation::SuperFilterTranslation(
    an<Translation> inner,
    const SuperConfig& config,
    std::shared_ptr<SuperBinaryDb> db,
    Context* ctx)
    : inner_(inner), cfg_(config), db_(db), ctx_(ctx) {

    size_t start = 0;
    size_t end = ctx_->input().length();
    if (inner_ && !inner_->exhausted()) {
        auto first_cand = inner_->Peek();
        if (first_cand) {
            start = first_cand->start();
            end = first_cand->end();
        }
    }

    std::string seg_input = ctx_->input().substr(start, end - start);
    GenerateAbbrevCandidates(seg_input, start, end);
    UpdateExhausted();
}

void SuperFilterTranslation::UpdateExhausted() {
    while (pending_candidates_.empty() && !inner_->exhausted()) {
        ProcessNextInner();
    }
    set_exhausted(index_cands_.empty() && quality_cands_.empty() &&
                  pending_candidates_.empty() && lazy_cands_.empty() && inner_->exhausted());
}

an<Candidate> SuperFilterTranslation::Peek() {
    if (exhausted()) return nullptr;

    if (!index_cands_.empty() && (yield_count_ + 1) >= index_cands_.front().value) {
        return index_cands_.front().cand;
    }
    if (!quality_cands_.empty()) {
        if (pending_candidates_.empty() || pending_candidates_.front()->quality() < quality_cands_.front().value) {
            return quality_cands_.front().cand;
        }
    }
    if (!pending_candidates_.empty()) return pending_candidates_.front();
    if (!index_cands_.empty()) return index_cands_.front().cand;
    if (!quality_cands_.empty()) return quality_cands_.front().cand;
    if (!lazy_cands_.empty()) return lazy_cands_.front();

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

std::string SuperFilterTranslation::SegmentConvert(const std::string& text, bool sentence) {
    if (!db_) return text;

    if (!sentence) {
        std::string val;
        if (db_->Fetch(text, &val)) {
            auto parts = Split(val, cfg_.delimiter);
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
            if (db_->Fetch(sub_text, &val)) {
                auto parts = Split(val, cfg_.delimiter);
                result += parts.empty() ? sub_text : parts[0];
                i = j;
                matched = true;
                break;
            }
        }
        if (!matched) {
            std::string single = text.substr(offsets[i], offsets[i+1] - offsets[i]);
            std::string val;
            if (db_->Fetch(single, &val)) {
                auto parts = Split(val, cfg_.delimiter);
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
    if (!db_ || cfg_.mode != "abbrev") return;
    abbrev_yielded_.clear();

    if (!cfg_.tags.empty()) {
        bool is_tag_match = false;
        size_t seg_start = 0;
        if (!ctx_->composition().empty()) {
            const auto& seg = ctx_->composition().back();
            seg_start = seg.start;
            for (const auto& req_tag : cfg_.tags) {
                if (seg.HasTag(req_tag)) { is_tag_match = true; break; }
            }
        }
        
        bool is_pure_chars = std::all_of(input_code.begin(), input_code.end(), 
                                         [](unsigned char c) { return c < 128 && std::isalnum(c); });

        if (!is_tag_match || seg_start != 0 || !is_pure_chars) return; 
    }

    bool is_active = cfg_.always_on;
    if (!is_active) {
        for (const auto& opt : cfg_.options) {
            if (ctx_->get_option(opt)) { is_active = true; break; }
        }
    }
    if (!is_active) return;

    std::string val;
    if (!db_->Fetch(input_code, &val)) {
        std::string upper_code = input_code;
        for (auto& c : upper_code) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        db_->Fetch(upper_code, &val);
    }

    if (!val.empty()) {
        auto parts = Split(val, cfg_.delimiter);
        int count = 0;
        for (const auto& p : parts) {
            std::string item_text = p;
            std::string item_preedit = input_code;

            if (abbrev_yielded_.count(item_text)) continue;
            abbrev_yielded_.insert(item_text);
            count++;

            auto cand = New<SimpleCandidate>(cfg_.cand_type, start, end, item_text, "");
            cand->set_preedit(item_preedit);

            if (count <= cfg_.always_qty) {
                if (cfg_.order_type == "index") {
                    cand->set_quality(999);
                    index_cands_.push_back({cand, cfg_.order_value + (count - 1)});
                } else if (cfg_.order_type == "quality") {
                    cand->set_quality(cfg_.order_value);
                    quality_cands_.push_back({cand, cfg_.order_value});
                }
            } else {
                cand->set_quality(0);
                lazy_cands_.push_back(cand);
            }
        }
    }
}

void SuperFilterTranslation::ProcessNextInner() {
    if (inner_->exhausted()) return;
    auto cand = inner_->Peek();
    inner_->Next();

    if (!cand) return;

    if (!db_ || cfg_.mode == "abbrev") {
        pending_candidates_.push_back(cand);
        return;
    }

    if (!cfg_.tags.empty()) {
        bool is_tag_match = false;
        if (!ctx_->composition().empty()) {
            const auto& seg = ctx_->composition().back();
            for (const auto& req_tag : cfg_.tags) {
                if (seg.HasTag(req_tag)) { is_tag_match = true; break; }
            }
        }
        if (!is_tag_match) {
            pending_candidates_.push_back(cand);
            return; 
        }
    }

    bool is_active = cfg_.always_on;
    if (!is_active) {
        for (const auto& opt : cfg_.options) {
            if (ctx_->get_option(opt)) { is_active = true; break; }
        }
    }
    
    if (!is_active) {
        pending_candidates_.push_back(cand);
        return;
    }

    std::string val;
    if (cfg_.sentence) {
        std::string fmm_res = SegmentConvert(cand->text(), true);
        if (fmm_res != cand->text()) val = fmm_res;
    } else {
        db_->Fetch(cand->text(), &val);
    }

    if (!val.empty()) {
        auto parts = Split(val, cfg_.delimiter);
        if (cfg_.t9_mode) {
            for (auto& p : parts) {
                size_t delim_pos = p.find("==");
                if (delim_pos != std::string::npos) p = p.substr(0, delim_pos);
            }
        }
        
        std::string rule_comment = "";
        if (cfg_.comment_mode == "text" && !cand->text().empty()) {
            std::string cfmt = cfg_.comment_format;
            size_t pos = cfmt.find("%s");
            if (pos != std::string::npos) {
                cfmt.replace(pos, 2, cand->text());
                rule_comment = cfmt;
            } else rule_comment = cand->text();
        } else if (cfg_.comment_mode == "append") {
            rule_comment = cand->comment();
        }

        if (cfg_.mode == "replace") {
            for (size_t i = 0; i < parts.size(); ++i) {
                std::string final_comment = (i == 0 && cfg_.comment_mode == "none") ? "" : rule_comment;
                std::string ctype = (i == 0) ? cand->type() : cfg_.cand_type;
                auto nc = New<SimpleCandidate>(ctype, cand->start(), cand->end(), parts[i], final_comment);
                nc->set_quality(cand->quality());
                nc->set_preedit(cand->preedit());
                pending_candidates_.push_back(nc);
            }
        } else if (cfg_.mode == "append") {
            pending_candidates_.push_back(cand);
            for (const auto& p : parts) {
                std::string final_comment = (cfg_.comment_mode == "none") ? "" : rule_comment;
                auto nc = New<SimpleCandidate>(cfg_.cand_type, cand->start(), cand->end(), p, final_comment);
                nc->set_quality(cand->quality());
                nc->set_preedit(cand->preedit());
                pending_candidates_.push_back(nc);
            }
        } else if (cfg_.mode == "comment") {
            std::string joined;
            for(size_t i = 0; i < parts.size(); ++i) joined += parts[i] + (i < parts.size() - 1 ? " " : ""); 
            std::string cfmt = cfg_.comment_format;
            size_t pos = cfmt.find("%s");
            if (pos != std::string::npos) cfmt.replace(pos, 2, joined);
            else cfmt = joined;
            
            std::string new_comment;
            if (cfg_.comment_mode == "none") new_comment = ""; 
            else if (cfg_.comment_mode == "text") new_comment = cfmt; 
            else new_comment = cand->comment() + cfmt; 
            
            auto nc = New<SimpleCandidate>(cand->type(), cand->start(), cand->end(), cand->text(), new_comment);
            nc->set_quality(cand->quality());
            nc->set_preedit(cand->preedit());
            pending_candidates_.push_back(nc);
        }
    } else {
        pending_candidates_.push_back(cand);
    }
}

// SuperFilter
SuperFilter::SuperFilter(const Ticket& ticket) 
    : Filter(ticket), name_space_(ticket.name_space.empty() ? "super_filter" : ticket.name_space) {
    if (ticket.schema) {
        LoadConfig(ticket.schema->config());
        InitializeBinaryDb();
    }
}

SuperFilter::~SuperFilter() {}

void SuperFilter::LoadConfig(Config* config) {
    config->GetString(name_space_ + "/db_name", &db_name_);
    if (!db_name_.empty()) db_name_ = std::filesystem::path(db_name_).filename().string();
    if (db_name_.empty() || db_name_ == "." || db_name_ == "..") db_name_ = name_space_; 

    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    db_name_ = user_dir + "/build/super_filter_" + db_name_ + ".bin";

    config->GetString(name_space_ + "/delimiter", &cfg_.delimiter);
    config->GetString(name_space_ + "/comment_format", &cfg_.comment_format);

    auto opt_node = config->GetItem(name_space_ + "/option");
    if (auto opt_val = As<ConfigValue>(opt_node)) {
        if (opt_val->str() == "true") cfg_.always_on = true;
        else if (opt_val->str() != "false") cfg_.options.push_back(opt_val->str());
    } else if (auto opt_list = As<ConfigList>(opt_node)) {
        for (size_t j = 0; j < opt_list->size(); ++j) {
            if (auto v = As<ConfigValue>(opt_list->GetAt(j))) cfg_.options.push_back(v->str());
        }
    }

    auto tag_node = config->GetItem(name_space_ + "/tags");
    if (!tag_node) tag_node = config->GetItem(name_space_ + "/tag");
    if (auto tag_val = As<ConfigValue>(tag_node)) {
        cfg_.tags.push_back(tag_val->str());
    } else if (auto tag_list = As<ConfigList>(tag_node)) {
        for (size_t j = 0; j < tag_list->size(); ++j) {
            if (auto v = As<ConfigValue>(tag_list->GetAt(j))) cfg_.tags.push_back(v->str());
        }
    }
    
    config->GetString(name_space_ + "/mode", &cfg_.mode);
    config->GetBool(name_space_ + "/sentence", &cfg_.sentence);
    config->GetString(name_space_ + "/comment_mode", &cfg_.comment_mode);
    config->GetString(name_space_ + "/cand_type", &cfg_.cand_type);
    
    if (auto t9_val = As<ConfigValue>(config->GetItem(name_space_ + "/t9_mode"))) {
        cfg_.t9_mode = (t9_val->str() == "true");
        if (cfg_.t9_mode && cfg_.mode != "abbrev") cfg_.t9_mode = false;
    }

    if (cfg_.mode == "abbrev") {
        std::string order_str;
        config->GetString(name_space_ + "/order", &order_str);
        if (!order_str.empty()) {
            auto parts = Split(order_str, ",");
            if (parts.size() >= 2) {
                try {
                    cfg_.order_type = parts[0];
                    cfg_.order_value = std::stoi(parts[1]);
                    if (parts.size() >= 3) cfg_.always_qty = std::stoi(parts[2]);
                } catch (...) {}
            }
        }
    } 

    auto files_node = config->GetItem(name_space_ + "/files");
    if (auto files_list = As<ConfigList>(files_node)) {
        for(size_t j = 0; j < files_list->size(); ++j) {
            if (auto f = As<ConfigValue>(files_list->GetAt(j))) {
                std::string filepath = f->str();
                if (!filepath.empty() && filepath.front() != '/' && filepath.front() != '\\' && filepath.find("..") == std::string::npos) {
                    cfg_.files.push_back(filepath);
                }
            }
        }
    }
}

std::string SuperFilter::GenerateFilesSignature() {
    std::string sig = "delim:" + cfg_.delimiter + "||";
    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    std::error_code ec_exist;

    sig += "t9:" + std::to_string(cfg_.t9_mode) + "@";
    for (const auto& path : cfg_.files) {
        sig += "path:" + path + "=";
        std::filesystem::path full_path = user_dir + "/" + path;
        
        if (std::filesystem::exists(full_path, ec_exist) && !ec_exist) {
            std::error_code ec_time, ec_size;
            auto ftime = std::filesystem::last_write_time(full_path, ec_time);
            auto fsize = std::filesystem::file_size(full_path, ec_size);
            
            if (!ec_time && !ec_size) {
                auto time_sec = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
                sig += std::to_string(fsize) + "_" + std::to_string(time_sec) + "|";
            }
        }
    }
    return sig;
}

void SuperFilter::InitializeBinaryDb() {
    if (cfg_.files.empty()) return;

    std::lock_guard<std::mutex> lock(g_db_cache_mutex);
    auto& cache_map = GetGlobalDbCache();
    std::string current_sig = GenerateFilesSignature();

    auto it = cache_map.find(db_name_);
    if (it != cache_map.end() && it->second.db && it->second.files_sig == current_sig) {
        db_ = it->second.db;
        return;
    }

    std::shared_ptr<SuperBinaryDb> new_db = std::make_shared<SuperBinaryDb>(db_name_);
    bool need_rebuild = !new_db->CheckSignature(current_sig);

    if (need_rebuild) {
        LOG(INFO) << "super_filter [" << name_space_ << "]: Rebuilding binary dictionary...";
        RebuildDb();
    }

    if (new_db->Open()) {
        SuperDbCacheItem item;
        item.db = new_db;
        item.files_sig = current_sig;
        cache_map[db_name_] = item;
        db_ = new_db;
    }
}

struct DictItem {
    std::string value;
    double weight;
    int order;
};

void SuperFilter::RebuildDb() {
    std::string user_dir = string(rime_get_api()->get_user_data_dir());
    std::error_code ec_dir;
    std::filesystem::create_directories(user_dir + "/build", ec_dir);

    std::map<std::string, std::string> final_binary_data;
    std::map<std::string, std::vector<DictItem>> merged_data;
    int line_counter = 0;

    for (const auto& path : cfg_.files) {
        std::string full_path = user_dir + "/" + path;
        std::ifstream file(full_path);
        if (!file.is_open()) continue;
        
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

                if (cfg_.t9_mode) {
                    for (char& c : key) {
                        if (c >= 'a' && c <= 'z') c = t9_map[c - 'a'];
                        else if (c >= 'A' && c <= 'Z') c = t9_map[c - 'A'];
                    }
                }

                size_t val_start = line.find_first_not_of(" \t", sep1);
                if (val_start != std::string::npos) {
                    std::string rest = line.substr(val_start);
                    rest.erase(rest.find_last_not_of("\r\n \t") + 1); 

                    std::string val = rest;
                    if (cfg_.t9_mode && val.find("==") == std::string::npos) {
                        val = val + "==" + orig_key;
                    }
                    
                    double weight = 0.0;
                    size_t last_delim = rest.find_last_of(" \t");
                    if (last_delim != std::string::npos) {
                        size_t weight_start = rest.find_first_not_of(" \t", last_delim);
                        if (weight_start != std::string::npos) {
                            std::string weight_str = rest.substr(weight_start);
                            try {
                                size_t parsed_len;
                                weight = std::stod(weight_str, &parsed_len);
                                if (parsed_len == weight_str.length()) {
                                    val = rest.substr(0, last_delim);
                                    val.erase(val.find_last_not_of(" \t") + 1);
                                } else weight = 0.0;
                            } catch (...) { weight = 0.0; }
                        }
                    }
                    merged_data[key].push_back({val, weight, line_counter++});
                }
            }
        }
    }

    for (auto& kv : merged_data) {
        auto& items = kv.second;
        std::sort(items.begin(), items.end(), [](const DictItem& a, const DictItem& b) {
            if (a.weight != b.weight) return a.weight > b.weight;
            return a.order < b.order;
        });

        std::string final_val;
        for (size_t i = 0; i < items.size(); ++i) {
            final_val += items[i].value;
            if (i < items.size() - 1) final_val += cfg_.delimiter;
        }
        final_binary_data[kv.first] = final_val;
    }

    std::string current_sig = GenerateFilesSignature();
    SuperBinaryDb::Build(db_name_, current_sig, final_binary_data);
}

an<Translation> SuperFilter::Apply(an<Translation> translation, CandidateList* candidates) {
    if (!translation) return nullptr;
    Context* ctx = engine_->context();
    if (!ctx->IsComposing() || ctx->input().empty()) return translation;
    return New<SuperFilterTranslation>(translation, cfg_, db_, ctx);
}

} // namespace rime