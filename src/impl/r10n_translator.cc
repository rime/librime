// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/r10n_translator.h>
#include <rime/impl/syllablizer.h>

namespace rime {

class R10nTranslation : public Translation {
 public:
  R10nTranslation(shared_ptr<DictEntryCollector>& collector,
                  int start)
      : collector_(collector), start_(start) {
    set_exhausted(!collector_ || collector_->empty());
  }

  virtual bool Next() {
    if (exhausted())
      return false;
    DictEntryIterator &iter(collector_->rbegin()->second);
    if (!iter.Next()) {
      int consumed_input_length = collector_->rbegin()->first;
      EZLOGGERVAR(consumed_input_length);
      collector_->erase(consumed_input_length);
      EZLOGGERVAR(collector_->size());
      set_exhausted(!collector_ || collector_->empty());
    }
    return exhausted();
  }

  virtual shared_ptr<Candidate> Peek() {
    if (exhausted())
      return shared_ptr<Candidate>();
    int consumed_input_length = collector_->rbegin()->first;
    EZLOGGERVAR(consumed_input_length);
    DictEntryIterator &iter(collector_->rbegin()->second);
    const shared_ptr<DictEntry> &e(iter.Peek());
    EZLOGGERVAR(e->text);
    shared_ptr<Candidate> cand(new Candidate(
        "zh",
        e->text,
        "",
        start_,
        start_ + consumed_input_length,
        0));
    return cand;
  }

 private:
  shared_ptr<DictEntryCollector> collector_;
  int start_;
};

R10nTranslator::R10nTranslator(Engine *engine)
    : Translator(engine) {
  if (!engine)
    return;
  Config *config = engine->schema()->config();
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified in schema '%s'.",
                  engine->schema()->schema_id().c_str());
    return;
  }
  dict_.reset(new Dictionary(dict_name));
  dict_->Load();
}

R10nTranslator::~R10nTranslator() {
}

Translation* R10nTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("abc"))
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);

  Syllablizer syllablizer;
  SyllableGraph syllable_graph;
  int consumed = syllablizer.BuildSyllableGraph(input,
                                                *dict_->prism(),
                                                &syllable_graph);

  Translation *translation = NULL;
  shared_ptr<DictEntryCollector> collector =
      dict_->Lookup(syllable_graph, 0);
  if (collector)
    translation = new R10nTranslation(collector, segment.start);
  return translation;
}

}  // namespace rime
