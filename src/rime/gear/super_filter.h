// librime/src/rime/gear/super_filter.h
#ifndef RIME_SUPER_FILTER_H_
#define RIME_SUPER_FILTER_H_

#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/filter.h>
#include <rime/translation.h>
#include <rime/dict/db.h>
#include <rime/context.h>
#include <rime/config.h>

namespace rime {

// Representation of a single filter rule configured in YAML.
struct SuperRule {
    std::string name;
    bool always_on = false;
    std::vector<std::string> options;

    std::string mode;         // Supported modes: append, replace, comment, abbrev
    bool sentence = false;    // Enable FMM (Forward Maximum Matching) for long phrases

    std::string prefix;
    std::vector<std::string> files;

    // 开启九宫格(T9)模式：构建词库时自动将编码转数字，并保留原编码为 preedit
    bool t9_mode = false;

    std::string cand_type = "derived";
    std::string comment_mode; // Supported modes: none, text, append

    // Injection parameters strictly for 'abbrev' mode
    std::string order_type = "index"; // 'index' (absolute position) or 'quality' (score threshold)
    int order_value = 1;
    int always_qty = 1;
};

// Wrapper for candidates that require forced injection at specific positions or quality thresholds.
struct InjectCand {
    an<Candidate> cand;
    int value;
};

// The core translation class implementing lazy evaluation and stream processing.
class SuperFilterTranslation : public Translation {
public:
    SuperFilterTranslation(an<Translation> inner,
                           const std::vector<SuperRule>& rules,
                           an<Db> db,
                           Context* ctx,
                           const std::string& delimiter,
                           const std::string& comment_format,
                           bool is_chain);

    an<Candidate> Peek() override;
    bool Next() override;

private:
    void GenerateAbbrevCandidates(const std::string& input_code, size_t start, size_t end);
    void ProcessNextInner();
    void UpdateExhausted();

    std::string SegmentConvert(const std::string& text, const std::string& prefix, bool sentence);

    an<Translation> inner_;
    std::vector<SuperRule> rules_;
    an<Db> db_;
    Context* ctx_;
    std::string delimiter_;
    std::string comment_format_;
    bool is_chain_;

    int yield_count_ = 0;

    // Priority queues for candidate distribution
    std::deque<InjectCand> index_cands_;
    std::deque<InjectCand> quality_cands_;
    std::deque<an<Candidate>> lazy_cands_;
    std::deque<an<Candidate>> pending_candidates_;
    std::unordered_set<std::string> abbrev_yielded_;
};

// Filter component responsible for parsing configurations and managing the global LevelDb connection.
class SuperFilter : public Filter {
public:
    explicit SuperFilter(const Ticket& ticket);
    virtual ~SuperFilter();

    an<Translation> Apply(an<Translation> translation,
                          CandidateList* candidates) override;

private:
    void LoadConfig(Config* config);
    void InitializeDb();
    std::string GenerateFilesSignature();
    void RebuildDb();

    std::vector<SuperRule> rules_;
    an<Db> db_;
    std::string db_name_;
    std::string delimiter_;
    std::string comment_format_;
    bool chain_ = false;
};

} // namespace rime

#endif // RIME_SUPER_FILTER_H_