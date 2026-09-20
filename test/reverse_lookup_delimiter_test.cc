//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Reverse lookup must honor schema-declared syllable delimiters
// (`speller/delimiter`). Regression: ReverseLookupTranslator hardcoded an
// empty delimiter set, so input like "xi'an" could not be syllabified and
// the lookup listed no candidates at all.
//
#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <fstream>
#include <string>

namespace {

constexpr const char* kSchemaYaml = R"YAML(# encoding: utf-8
schema:
  schema_id: reverse_lookup_delimiter_test
  name: reverse lookup delimiter test
engine:
  processors: [recognizer, speller]
  segmentors: [matcher, abc_segmentor]
  translators: [reverse_lookup_translator]
speller:
  alphabet: "abcdefghijklmnopqrstuvwxyz'"
  delimiter: " '"
recognizer:
  patterns:
    reverse_lookup: "^`[a-z']*$"
reverse_lookup:
  dictionary: dictionary_test
  prefix: "`"
)YAML";

// Types |keys| in a fresh session and returns the menu texts joined by '|'.
std::string Candidates(const char* keys) {
  RimeApi* api = rime_get_api();
  RimeSessionId session = api->create_session();
  EXPECT_NE(0, session);
  EXPECT_TRUE(api->select_schema(session, "reverse_lookup_delimiter_test"));
  for (const char* p = keys; *p; ++p) {
    api->process_key(session, static_cast<unsigned char>(*p), 0);
  }
  std::string result;
  RIME_STRUCT(RimeContext, ctx);
  if (api->get_context(session, &ctx)) {
    for (int i = 0; i < ctx.menu.num_candidates; ++i) {
      if (i) {
        result += '|';
      }
      result += ctx.menu.candidates[i].text ? ctx.menu.candidates[i].text : "";
    }
    api->free_context(&ctx);
  }
  api->destroy_session(session);
  return result;
}

}  // namespace

TEST(ReverseLookupTranslator, HonorsSchemaDelimiter) {
  // Compile the shared pinyin fixture (dictionary_test.dict.yaml contains
  // "西安 xi an") into the working directory, which the test environment
  // uses as the deployed resource root.
  rime::the<rime::Dictionary> dict(new rime::Dictionary(
      "dictionary_test", {},
      {rime::New<rime::Table>(rime::path{"dictionary_test.table.bin"})},
      rime::New<rime::Prism>(rime::path{"dictionary_test.prism.bin"})));
  dict->Remove();
  rime::DictCompiler compiler(dict.get());
  ASSERT_TRUE(compiler.Compile(rime::path()));
  ASSERT_TRUE(dict->Load());

  std::ofstream("reverse_lookup_delimiter_test.schema.yaml") << kSchemaYaml;

  EXPECT_NE(std::string::npos, Candidates("`xian").find("西安"));
  // Before the fix a delimiter in the input terminated the lookup.
  EXPECT_NE(std::string::npos, Candidates("`xi'an").find("西安"));
}
