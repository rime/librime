//
// Copyright RIME Developers
// Distributed under the BSD License
//
// commit_script_text and commit_comment go through commit_notifier, so
// listeners see the committed text and the composition to be memorized.
//
#include <gtest/gtest.h>
#include <rime/candidate.h>
#include <rime/component.h>
#include <rime/config/config_types.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/registry.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/translator.h>

using namespace rime;

namespace {

// Two candidates per segment: one with a comment and a preedit, one without.
class CommitTestTranslator : public Translator {
 public:
  explicit CommitTestTranslator(const Ticket& ticket) : Translator(ticket) {}

  an<Translation> Query(const string& input, const Segment& segment) override {
    auto translation = New<FifoTranslation>();
    translation->Append(New<SimpleCandidate>(
        "test", segment.start, segment.end, "text:" + input, "comment:" + input,
        "preedit:" + input));
    translation->Append(New<SimpleCandidate>("test", segment.start, segment.end,
                                             "plain:" + input));
    return translation;
  }
};

struct CommitEvent {
  string commit_text;
  // selected candidates left in the composition, which Memory would learn.
  vector<string> composition;
};

class EditorCommitTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Registry::instance().Register("commit_test_translator",
                                  new Component<CommitTestTranslator>);
    engine_.reset(Engine::Create());

    Config* config = engine_->schema()->config();
    config->SetString("schema/schema_id", "editor_commit_test");
    SetList(config, "engine/processors", "fluid_editor");
    SetList(config, "engine/segmentors", "abc_segmentor");
    SetList(config, "engine/translators", "commit_test_translator");
    engine_->ApplySchema(engine_->schema());

    engine_->context()->commit_notifier().connect([this](Context* ctx) {
      CommitEvent event{ctx->GetCommitText(), {}};
      for (const auto& seg : ctx->composition()) {
        if (auto cand = seg.GetSelectedCandidate())
          event.composition.push_back(cand->text());
      }
      commits_.push_back(event);
    });
    engine_->sink().connect(
        [this](const string& text) { sink_.push_back(text); });
  }

  void TearDown() override {
    engine_.reset();
    Registry::instance().Unregister("commit_test_translator");
  }

  static void SetList(Config* config, const string& path, const string& name) {
    auto list = New<ConfigList>();
    list->Append(New<ConfigValue>(name));
    config->SetItem(path, list);
  }

  void Type(const string& keys) {
    for (char ch : keys)
      ASSERT_TRUE(engine_->ProcessKey(KeyEvent(ch, 0)));
  }

  // types "ni", confirms "text:ni", then types "hao".
  void ComposeWithSelection() {
    Type("ni");
    ASSERT_TRUE(engine_->ProcessKey(KeyEvent(XK_space, 0)));
    Type("hao");
  }

  the<Engine> engine_;
  vector<CommitEvent> commits_;
  vector<string> sink_;
};

}  // namespace

TEST_F(EditorCommitTest, ScriptTextIsCommittedThroughNotifier) {
  Type("nihao");
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_Return, kShiftMask)));

  ASSERT_EQ(commits_.size(), 1u);
  EXPECT_EQ(commits_[0].commit_text, "preedit:nihao");
  EXPECT_TRUE(commits_[0].composition.empty());
  EXPECT_EQ(sink_, vector<string>{"preedit:nihao"});
  EXPECT_FALSE(engine_->context()->IsComposing());
}

TEST_F(EditorCommitTest, ScriptTextKeepsOnlySelectedSegments) {
  ComposeWithSelection();
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_Return, kShiftMask)));

  ASSERT_EQ(commits_.size(), 1u);
  EXPECT_EQ(commits_[0].commit_text, "text:nipreedit:hao");
  // the highlighted but unselected "text:hao" is not to be memorized.
  EXPECT_EQ(commits_[0].composition, vector<string>{"text:ni"});
  EXPECT_EQ(sink_, vector<string>{"text:nipreedit:hao"});
  EXPECT_FALSE(engine_->context()->IsComposing());
}

TEST_F(EditorCommitTest, CommentIsCommittedThroughNotifier) {
  ComposeWithSelection();
  EXPECT_TRUE(
      engine_->ProcessKey(KeyEvent(XK_Return, kControlMask | kShiftMask)));

  ASSERT_EQ(commits_.size(), 1u);
  EXPECT_EQ(commits_[0].commit_text, "comment:hao");
  // nothing is to be memorized when only the comment is committed.
  EXPECT_TRUE(commits_[0].composition.empty());
  EXPECT_EQ(sink_, vector<string>{"comment:hao"});
  EXPECT_FALSE(engine_->context()->IsComposing());
}

TEST_F(EditorCommitTest, EmptyCommentCommitsNothing) {
  Type("nihao");
  ASSERT_TRUE(engine_->context()->Highlight(1));
  EXPECT_TRUE(
      engine_->ProcessKey(KeyEvent(XK_Return, kControlMask | kShiftMask)));

  EXPECT_TRUE(commits_.empty());
  EXPECT_TRUE(sink_.empty());
  EXPECT_EQ(engine_->context()->input(), "nihao");
}
