#include <gtest/gtest.h>
#include <rime/config/config_types.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/filter.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/processor.h>
#include <rime/registry.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/segmentor.h>
#include <rime/translation.h>

using namespace rime;

namespace {

class CountingProcessor : public Processor {
 public:
  explicit CountingProcessor(const Ticket& ticket) : Processor(ticket) {}

  ProcessResult ProcessKeyEvent(const KeyEvent&) override {
    ++calls;
    return kNoop;
  }

  static int calls;
};

int CountingProcessor::calls = 0;

class CountingSegmentor : public Segmentor {
 public:
  explicit CountingSegmentor(const Ticket& ticket) : Segmentor(ticket) {}

  bool Proceed(Segmentation*) override {
    ++calls;
    return true;
  }

  static int calls;
};

int CountingSegmentor::calls = 0;

class CountingFilter : public Filter {
 public:
  explicit CountingFilter(const Ticket& ticket) : Filter(ticket) {}

  an<Translation> Apply(an<Translation> translation, CandidateList*) override {
    ++calls;
    return translation;
  }

  static int calls;
};

int CountingFilter::calls = 0;

void SetComponentList(Config* config,
                      const string& path,
                      const string& name,
                      const string& when = string()) {
  auto list = New<ConfigList>();
  auto item = New<ConfigMap>();
  item->Set("name", New<ConfigValue>(name));
  if (!when.empty())
    item->Set("when", New<ConfigValue>(when));
  list->Append(item);
  config->SetItem(path, list);
}

void SetCombinedCondition(Config* config,
                          const string& path,
                          const string& operation,
                          const vector<pair<string, bool>>& conditions) {
  auto list = New<ConfigList>();
  auto component = New<ConfigMap>();
  component->Set("name", New<ConfigValue>("test_counting_processor"));

  auto condition_list = New<ConfigList>();
  for (const auto& condition_spec : conditions) {
    auto condition = New<ConfigMap>();
    condition->Set("option", New<ConfigValue>(condition_spec.first));
    condition->Set("value", New<ConfigValue>(condition_spec.second));
    condition_list->Append(condition);
  }
  auto condition = New<ConfigMap>();
  condition->Set(operation, condition_list);
  component->Set("condition", condition);
  list->Append(component);
  config->SetItem(path, list);
}

class ConditionalComponentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Registry::instance().Register("test_counting_processor",
                                  new Component<CountingProcessor>);
    Registry::instance().Register("test_counting_segmentor",
                                  new Component<CountingSegmentor>);
    Registry::instance().Register("test_counting_filter",
                                  new Component<CountingFilter>);
    CountingProcessor::calls = 0;
    CountingSegmentor::calls = 0;
    CountingFilter::calls = 0;
    engine_.reset(Engine::Create());
  }

  void TearDown() override {
    engine_.reset();
    Registry::instance().Unregister("test_counting_processor");
    Registry::instance().Unregister("test_counting_segmentor");
    Registry::instance().Unregister("test_counting_filter");
  }

  the<Engine> engine_;
};

}  // namespace

TEST_F(ConditionalComponentTest, ProcessorCanBeEnabledAndDisabledByOption) {
  Config* config = engine_->schema()->config();
  SetComponentList(config, "engine/processors", "test_counting_processor",
                   "option_0");
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);

  engine_->context()->set_option("option_0", false);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);
}

TEST_F(ConditionalComponentTest, TranslatorAndFilterFollowOptionChanges) {
  Config* config = engine_->schema()->config();

  auto segmentors = New<ConfigList>();
  segmentors->Append(New<ConfigValue>("abc_segmentor"));
  config->SetItem("engine/segmentors", segmentors);

  auto translators = New<ConfigList>();
  auto translator = New<ConfigMap>();
  translator->Set("name", New<ConfigValue>("echo_translator"));
  translator->Set("when", New<ConfigValue>("option_0"));
  translators->Append(translator);
  config->SetItem("engine/translators", translators);

  SetComponentList(config, "engine/filters", "test_counting_filter",
                   "option_0");
  engine_->ApplySchema(engine_->schema());

  engine_->context()->set_input("a");
  ASSERT_EQ(engine_->context()->composition().size(), 1u);
  EXPECT_FALSE(engine_->context()->HasMenu());
  EXPECT_EQ(CountingFilter::calls, 0);

  engine_->context()->set_option("option_0", true);
  ASSERT_EQ(engine_->context()->composition().size(), 1u);
  EXPECT_TRUE(engine_->context()->HasMenu());
  EXPECT_EQ(CountingFilter::calls, 1);

  engine_->context()->set_option("option_0", false);
  ASSERT_EQ(engine_->context()->composition().size(), 1u);
  EXPECT_FALSE(engine_->context()->HasMenu());
  EXPECT_EQ(CountingFilter::calls, 1);
}

TEST_F(ConditionalComponentTest, NegatedConditionIsSupported) {
  Config* config = engine_->schema()->config();
  SetComponentList(config, "engine/processors", "test_counting_processor",
                   "!option_0");
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);
}

TEST_F(ConditionalComponentTest, SegmentorCanBeSkippedByOption) {
  Config* config = engine_->schema()->config();
  SetComponentList(config, "engine/segmentors", "test_counting_segmentor",
                   "option_0");
  engine_->ApplySchema(engine_->schema());

  engine_->context()->set_input("a");
  EXPECT_EQ(CountingSegmentor::calls, 0);

  engine_->context()->set_option("option_0", true);
  engine_->context()->set_input("a");
  EXPECT_EQ(CountingSegmentor::calls, 1);

  engine_->context()->set_option("option_0", false);
  engine_->context()->set_input("a");
  EXPECT_EQ(CountingSegmentor::calls, 1);
}

TEST_F(ConditionalComponentTest, AllConditionRequiresEveryOption) {
  SetCombinedCondition(engine_->schema()->config(), "engine/processors", "all",
                       {{"option_0", true}, {"option_1", false}});
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);

  engine_->context()->set_option("option_1", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);

  engine_->context()->set_option("option_1", false);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 2);
}

TEST_F(ConditionalComponentTest, AnyConditionRequiresOneMatchingOption) {
  SetCombinedCondition(engine_->schema()->config(), "engine/processors", "any",
                       {{"option_0", true}, {"option_1", true}});
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);

  engine_->context()->set_option("option_1", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 1);

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 2);

  engine_->context()->set_option("option_1", false);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 3);

  engine_->context()->set_option("option_0", false);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 3);
}

TEST_F(ConditionalComponentTest, InvalidWhenTypeDoesNotEnableComponent) {
  auto processors = New<ConfigList>();
  auto conditional_processor = New<ConfigMap>();
  conditional_processor->Set("name",
                             New<ConfigValue>("test_counting_processor"));
  auto invalid_when = New<ConfigList>();
  invalid_when->Append(New<ConfigValue>("option_0"));
  conditional_processor->Set("when", invalid_when);
  processors->Append(conditional_processor);
  engine_->schema()->config()->SetItem("engine/processors", processors);
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);
}

TEST_F(ConditionalComponentTest, EmptyWhenDoesNotEnableComponent) {
  auto processors = New<ConfigList>();
  auto conditional_processor = New<ConfigMap>();
  conditional_processor->Set("name",
                             New<ConfigValue>("test_counting_processor"));
  conditional_processor->Set("when", New<ConfigValue>(""));
  processors->Append(conditional_processor);
  engine_->schema()->config()->SetItem("engine/processors", processors);
  engine_->ApplySchema(engine_->schema());

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);
}

TEST_F(ConditionalComponentTest, UnknownConditionKeyDoesNotEnableComponent) {
  auto processors = New<ConfigList>();
  auto conditional_processor = New<ConfigMap>();
  conditional_processor->Set("name",
                             New<ConfigValue>("test_counting_processor"));
  auto option_condition = New<ConfigMap>();
  option_condition->Set("option", New<ConfigValue>("option_0"));
  option_condition->Set("value", New<ConfigValue>(true));
  option_condition->Set("vale", New<ConfigValue>(true));
  auto condition_list = New<ConfigList>();
  condition_list->Append(option_condition);
  auto condition = New<ConfigMap>();
  condition->Set("all", condition_list);
  conditional_processor->Set("condition", condition);
  processors->Append(conditional_processor);
  engine_->schema()->config()->SetItem("engine/processors", processors);
  engine_->ApplySchema(engine_->schema());

  engine_->context()->set_option("option_0", true);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_a, 0)));
  EXPECT_EQ(CountingProcessor::calls, 0);
}
