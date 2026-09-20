//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Reverse lookup must honor the schema-declared syllable delimiters
// (`speller/delimiter`). Regression: ReverseLookupTranslator built its
// Syllabifier with a hardcoded empty delimiter set, so any delimiter in the
// input (e.g. the customary `'` between pinyin syllables) could not be
// consumed, syllabification stopped before the end of the input, and the
// lookup returned no candidates at all.
//
#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/dict/dict_compiler.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// A tiny pinyin fixture: one multi-syllable word spelled "xi an" plus
// single-syllable entries that share the joined spelling "xian".
constexpr const char* kDictionaryYaml =
    "---\n"
    "name: reverse_lookup_delimiter\n"
    "version: \"1\"\n"
    "sort: by_weight\n"
    "columns:\n"
    "  - text\n"
    "  - code\n"
    "  - weight\n"
    "...\n"
    "\n"
    "西安\txi an\t5000\n"
    "西岸\txi an\t3000\n"
    "先\txian\t5000\n"
    "线\txian\t3000\n"
    "你\tni\t5000\n";

// The schema under test: pinyin reverse lookup with the conventional
// apostrophe syllable delimiter declared by pinyin schemas.
constexpr const char* kTestSchemaYaml = R"YAML(# encoding: utf-8
schema:
  schema_id: reverse_lookup_delimiter_test
  name: reverse lookup delimiter test
  version: "1"
switches:
  - name: ascii_mode
    reset: 0
    states: [ 中文, 西文 ]
menu:
  page_size: 9
engine:
  processors:
    - ascii_composer
    - recognizer
    - speller
    - selector
    - navigator
    - express_editor
  segmentors:
    - matcher
    - abc_segmentor
    - fallback_segmentor
  translators:
    - reverse_lookup_translator
speller:
  alphabet: "abcdefghijklmnopqrstuvwxyz'"
  delimiter: " '"
recognizer:
  patterns:
    reverse_lookup: "^`[a-z']*$"
reverse_lookup:
  dictionary: reverse_lookup_delimiter
  prefix: "`"
)YAML";

void WriteFile(const fs::path& path, const char* content) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  ASSERT_TRUE(out) << "cannot write " << path;
  out << content;
}

// Types a key sequence in a fresh session and joins the current menu's
// candidate texts with '|'.
std::string CandidatesFor(const std::string& keys) {
  RimeApi* api = rime_get_api();
  RimeSessionId session = api->create_session();
  EXPECT_NE(0, session);
  if (!session) {
    return {};
  }
  EXPECT_TRUE(api->select_schema(session, "reverse_lookup_delimiter_test"));
  api->set_option(session, "ascii_mode", False);
  for (const char ch : keys) {
    api->process_key(session, static_cast<unsigned char>(ch), 0);
  }
  std::string result;
  RIME_STRUCT(RimeContext, ctx);
  if (api->get_context(session, &ctx)) {
    for (int i = 0; i < ctx.menu.num_candidates; ++i) {
      if (i) {
        result += '|';
      }
      const char* text = ctx.menu.candidates[i].text;
      result += text ? text : "";
    }
    api->free_context(&ctx);
  }
  api->destroy_session(session);
  return result;
}

}  // namespace

TEST(ReverseLookupTranslator, HonorsSchemaDelimiter) {
  // The test environment uses the working directory as the deployed resource
  // root. Compile the fixture dictionary into it (same pattern as
  // dictionary_test) and drop the compiled schema next to it.
  const fs::path cwd = fs::current_path();
  WriteFile(cwd / "reverse_lookup_delimiter.dict.yaml", kDictionaryYaml);
  {
    rime::the<rime::Dictionary> dict(new rime::Dictionary(
        "reverse_lookup_delimiter", {},
        {rime::New<rime::Table>(
            rime::path{"reverse_lookup_delimiter.table.bin"})},
        rime::New<rime::Prism>(
            rime::path{"reverse_lookup_delimiter.prism.bin"})));
    dict->Remove();
    rime::DictCompiler compiler(dict.get());
    ASSERT_TRUE(compiler.Compile(rime::path()));
    ASSERT_TRUE(dict->Load());
  }
  WriteFile(cwd / "reverse_lookup_delimiter_test.schema.yaml", kTestSchemaYaml);

  // Plain pinyin keeps working, joined or not.
  EXPECT_NE(std::string::npos, CandidatesFor("`xian").find("先"));
  EXPECT_NE(std::string::npos, CandidatesFor("`ni").find("你"));

  // Regression: the declared delimiter must split "xi'an" into two syllables
  // and still yield the multi-syllable word. Before the fix this returned an
  // empty candidate list.
  EXPECT_NE(std::string::npos, CandidatesFor("`xi'an").find("西安"));
}
