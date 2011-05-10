// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//
#include <sstream>
#include <gtest/gtest.h>
#include <rime/trivial_dictionary.h>
#include <rime/context.h>

using namespace rime;

TEST(TrivialDictionaryTest, Lookup) {
  Context *context=new Context();
  context->set_input("test");
  DictionaryResult *dict_result=new DictionaryResult();
  TrivialDictionary *trivial_dict=new TrivialDictionary();
  trivial_dict->Lookup(context,dict_result);
  std::string result=dict_result->result();
  EXPECT_EQ("test",result);
}

