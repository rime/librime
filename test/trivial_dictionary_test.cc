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
#include <rime/dictionary.h>

using namespace rime;

TEST(TrivialDictionaryTest, Lookup) {
  // make sure the component has been registered
  Dictionary::Component *component =
      Dictionary::Require("trivial_dictionary");
  ASSERT_TRUE(component);
  // make sure the dict object has been created
  Dictionary *trivial_dict = component->Create(NULL);
  ASSERT_TRUE(trivial_dict);
  // lookup test
  Context *context = new Context();
  context->set_input("test");
  DictionaryResult *dict_result = new DictionaryResult();
  trivial_dict->Lookup(context, dict_result);
  std::string result = dict_result->result();
  EXPECT_EQ("test", result);
  delete context;
  delete dict_result;
  delete trivial_dict;
}

