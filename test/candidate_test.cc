//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/candidate.h>

using namespace rime;

TEST(RimeCandidateTest, GetGenuineCandidate) {
  auto original = New<SimpleCandidate>("original", 0, 1, "a");
  auto shadow1 = New<ShadowCandidate>(original, "shadow1", "b");
  auto shadow2 = New<ShadowCandidate>(shadow1, "shadow2", "c");
  auto genuine = Candidate::GetGenuineCandidate(shadow2);
  ASSERT_EQ(genuine->type(), original->type());
  ASSERT_EQ(genuine->text(), original->text());
  ASSERT_EQ(genuine->start(), original->start());
  ASSERT_EQ(genuine->end(), original->end());
  ASSERT_EQ(genuine->comment(), original->comment());
}
