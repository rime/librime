//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/composition.h>
#include <rime/context.h>

using namespace rime;

TEST(ContextTest, DeleteInputClearsTabState) {
  Context context;
  context.set_input("nihao");
  context.AddTabConstraint(0, "n", 1);
  context.PushTabCursor(1);
  context.set_caret_pos(1);

  ASSERT_TRUE(context.DeleteInput());
  EXPECT_TRUE(context.tab_constraints().empty());
  EXPECT_EQ(0u, context.CurrentTabCursor());
  EXPECT_EQ("nhao", context.shadow_input());
}

TEST(ContextTest, AddTabConstraintAlwaysNotifiesUpdate) {
  Context context;
  context.set_input("ni");

  Composition comp;
  comp.Reset(context.input());
  Segment selected(0, 2);
  selected.status = Segment::kSelected;
  comp.AddSegment(selected);
  context.set_composition(std::move(comp));

  int updates = 0;
  auto conn = context.update_notifier().connect([&](Context*) { ++updates; });

  context.AddTabConstraint(0, "ni", 2);
  EXPECT_EQ(1, updates);

  conn.disconnect();
}
