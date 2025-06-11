#include <rime/testing.h>
#include <rime/engine.h>
#include <rime/schema.h>

TEST(RimeTibetanTest, WylieTransliteration) {
  rime::Engine engine(std::make_shared<rime::Schema>("tibetan_wylie"));
  
  // Test basic consonants
  EXPECT_EQ(engine.ProcessInput("ka").commit_text(), "ཀ");
  EXPECT_EQ(engine.ProcessInput("kha").commit_text(), "ཁ");
  
  // Test common words
  EXPECT_EQ(engine.ProcessInput("bod").commit_text(), "བོད་");
  EXPECT_EQ(engine.ProcessInput("skad").commit_text(), "སྐད་");
  
  // Test punctuation
  EXPECT_EQ(engine.ProcessInput(",").commit_text(), "་");
  EXPECT_EQ(engine.ProcessInput(".").commit_text(), "།");
}
