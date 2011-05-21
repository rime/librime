// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/context.h>
#include <rime/translator.h>

using namespace rime;

TEST(TrivialTranslatorTest, Query) {
  // make sure the component has been registered
  Translator::Component *component =
      Translator::Require("trivial_translator");
  ASSERT_TRUE(component);
  // make sure the dict object has been created
  Translator *translator = component->Create(NULL);
  ASSERT_TRUE(translator);
  // lookup test
  Context *context = new Context();
  context->set_input("test");
  Translation *translation = new Translation();
  translator->Query(context, translation);
  std::string result = translation->result();
  EXPECT_EQ("test", result);
  delete context;
  delete translation;
  delete translator;
}

