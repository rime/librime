//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-05-26 GONG Chen <chen.sst@gmail.com>
//

#include <ctime>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/translation.h>
#include <rime/gear/schema_list_translator.h>

namespace rime {

class SchemaSelection : public SimpleCandidate, public SwitcherCommand {
 public:
  SchemaSelection(Schema *schema)
      : SimpleCandidate("schema", 0, 0, schema->schema_name()),
        SwitcherCommand(schema->schema_id()) {
  }
  virtual void Apply(Switcher* switcher);
};

void SchemaSelection::Apply(Switcher* switcher) {
  switcher->Deactivate();
  if (Config* user_config = switcher->user_config()) {
    user_config->SetString("var/previously_selected_schema", keyword_);
    user_config->SetInt("var/schema_access_time/" + keyword_, time(NULL));
  }
  if (Engine* engine = switcher->attached_engine()) {
    if (keyword_ != engine->schema()->schema_id()) {
      engine->ApplySchema(new Schema(keyword_));
    }
  }
}

class SchemaAction : public ShadowCandidate, public SwitcherCommand {
 public:
  SchemaAction(shared_ptr<Candidate> schema,
               shared_ptr<Candidate> command)
      : ShadowCandidate(schema, command->type()),
        SwitcherCommand(As<SwitcherCommand>(schema)->keyword()),
        command_(As<SwitcherCommand>(command)) {
  }
  virtual void Apply(Switcher* switcher);

 private:
  shared_ptr<SwitcherCommand> command_;
};

void SchemaAction::Apply(Switcher* switcher) {
  if (command_) {
    command_->Apply(switcher);
  }
}

class SchemaListTranslation : public FifoTranslation {
 public:
  SchemaListTranslation(Switcher* switcher) {
    LoadSchemaList(switcher);
  }
  virtual int Compare(shared_ptr<Translation> other,
                      const CandidateList &candidates);

 protected:
  void LoadSchemaList(Switcher* switcher);
};

int SchemaListTranslation::Compare(shared_ptr<Translation> other,
                                   const CandidateList &candidates) {
  if (!other || other->exhausted()) return -1;
  if (exhausted()) return 1;
  // switches should immediately follow current schema (#0)
  shared_ptr<Candidate>  theirs = other->Peek();
  if (theirs && theirs->type() == "unfold") {
    if (cursor_ == 0) {
      // unfold its options when the current schema is selected
      candies_[0] = New<SchemaAction>(candies_[0], theirs);
    }
    return cursor_ == 0 ? -1 : 1;
  }
  if (theirs && theirs->type() == "switch") {
    return cursor_ == 0 ? -1 : 1;
  }
  return Translation::Compare(other, candidates);
}

static bool compare_access_time(const shared_ptr<Candidate>& a,
                                const shared_ptr<Candidate>& b) {
  return a->quality() > b->quality();
}

void SchemaListTranslation::LoadSchemaList(Switcher* switcher) {
  Engine* engine = switcher->attached_engine();
  if (!engine) return;
  Config *config = switcher->schema()->config();
  if (!config) return;
  ConfigListPtr schema_list = config->GetList("schema_list");
  if (!schema_list) return;
  // current schema comes first
  Schema* current_schema = engine->schema();
  if (current_schema) {
    Append(make_shared<SchemaSelection>(current_schema));
  }
  size_t fixed = candies_.size();
  Config* user_config = switcher->user_config();
  time_t now = time(NULL);
  // load the rest schema list
  for (size_t i = 0; i < schema_list->size(); ++i) {
    ConfigMapPtr item = As<ConfigMap>(schema_list->GetAt(i));
    if (!item) continue;
    ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    if (current_schema && schema_id == current_schema->schema_id())
      continue;
    Schema schema(schema_id);
    shared_ptr<Candidate> cand = make_shared<SchemaSelection>(&schema);
    int timestamp = 0;
    if (user_config &&
        user_config->GetInt("var/schema_access_time/" + schema_id,
                            &timestamp)) {
      if (timestamp <= now)
        cand->set_quality(timestamp);
    }
    Append(cand);
  }
  DLOG(INFO) << "num schemata: " << candies_.size();
  bool fix_order = false;
  config->GetBool("switcher/fix_schema_list_order", &fix_order);
  if (fix_order)
    return;
  // reorder schema list by recency
  std::stable_sort(candies_.begin() + fixed, candies_.end(),
                   compare_access_time);
}

SchemaListTranslator::SchemaListTranslator(const Ticket& ticket)
    : Translator(ticket) {
}

shared_ptr<Translation> SchemaListTranslator::Query(const std::string& input,
                                                    const Segment& segment,
                                                    std::string* prompt) {
  Switcher* switcher = dynamic_cast<Switcher*>(engine_);
  if (!switcher) {
    return shared_ptr<Translation>();
  }
  return make_shared<SchemaListTranslation>(switcher);
}

}  // namespace rime
