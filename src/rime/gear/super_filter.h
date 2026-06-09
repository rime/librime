// librime/src/rime/gear/super_filter.h
#ifndef RIME_SUPER_FILTER_H_
#define RIME_SUPER_FILTER_H_

#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <map>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/filter.h>
#include <rime/translation.h>
#include <rime/context.h>
#include <rime/config.h>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace rime {

struct SuperConfig {
    bool always_on = false;
    std::vector<std::string> options;
    std::vector<std::string> tags;

    std::string mode = "append";
    bool sentence = false;
    std::vector<std::string> files;
    bool t9_mode = false;

    std::string cand_type = "derived";
    std::string comment_mode = "none"; 
    std::string order_type = "index"; 
    int order_value = 1;
    int always_qty = 1;

    std::string delimiter = "|";
    std::string comment_format = "〔%s〕";
};

struct InjectCand {
    an<Candidate> cand;
    int value;
};

class SuperBinaryDb {
public:
    SuperBinaryDb(const std::string& bin_path);
    ~SuperBinaryDb();

    bool Open();
    void Close();
    bool Fetch(const std::string& key, std::string* value) const;
    bool CheckSignature(const std::string& expected_sig) const;

    static bool Build(const std::string& bin_path, const std::string& signature,
                      const std::map<std::string, std::string>& data);

private:
    std::string bin_path_;
    std::unique_ptr<boost::interprocess::file_mapping> mapping_;
    std::unique_ptr<boost::interprocess::mapped_region> region_;
    const char* data_ptr_ = nullptr;
    size_t data_size_ = 0;
    uint32_t record_count_ = 0;
    const uint32_t* index_ptr_ = nullptr;
};

class SuperFilterTranslation : public Translation {
public:
    SuperFilterTranslation(an<Translation> inner,
                           const SuperConfig& config,
                           std::shared_ptr<SuperBinaryDb> db,
                           Context* ctx);

    an<Candidate> Peek() override;
    bool Next() override;

private:
    void GenerateAbbrevCandidates(const std::string& input_code, size_t start, size_t end);
    void ProcessNextInner();
    void UpdateExhausted();

    std::string SegmentConvert(const std::string& text, bool sentence);

    an<Translation> inner_;
    SuperConfig cfg_;
    std::shared_ptr<SuperBinaryDb> db_;
    Context* ctx_;

    int yield_count_ = 0;

    std::deque<InjectCand> index_cands_;
    std::deque<InjectCand> quality_cands_;
    std::deque<an<Candidate>> lazy_cands_;
    std::deque<an<Candidate>> pending_candidates_;
    std::unordered_set<std::string> abbrev_yielded_;
};

class SuperFilter : public Filter {
public:
    explicit SuperFilter(const Ticket& ticket);
    virtual ~SuperFilter();

    an<Translation> Apply(an<Translation> translation, CandidateList* candidates) override;

private:
    void LoadConfig(Config* config);
    void InitializeBinaryDb();
    std::string GenerateFilesSignature();
    void RebuildDb();

    SuperConfig cfg_;
    std::shared_ptr<SuperBinaryDb> db_;
    std::string db_name_;
    std::string name_space_; 
};

class SuperFilterComponent : public SuperFilter::Component {
public:
    SuperFilter* Create(const Ticket& ticket) override {
        return new SuperFilter(ticket);
    }
};

} // namespace rime

#endif // RIME_SUPER_FILTER_H_