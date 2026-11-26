// Copyright 2026 Boyuan Yang <byang@debian.org>

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/dict/string_table.h>
#include <rime/dict/table.h>

namespace {

struct alignas(8) ImageBlock {
  char bytes[8];
};

std::vector<ImageBlock> MakeImage(size_t* image_size) {
  rime::StringTableBuilder builder;
  builder.Add("alpha");
  builder.Add("beta");
  builder.Build();
  *image_size = builder.BinarySize();
  std::vector<ImageBlock> image((*image_size + 7) / 8);
  builder.Dump(reinterpret_cast<char*>(image.data()), *image_size);
  return image;
}

class ByteAllocator : public rime::MappedFile {
 public:
  explicit ByteAllocator(const rime::path& file) : MappedFile(file) {}
  using MappedFile::Allocate;
  using MappedFile::AllocateAlignedBytes;
  using MappedFile::Create;
};

}  // namespace

TEST(RimeStringTableTest, AlignedImage) {
  size_t image_size;
  auto image = MakeImage(&image_size);
  rime::StringTable table(reinterpret_cast<const char*>(image.data()),
                          image_size);
  EXPECT_TRUE(table.HasKey("alpha"));
  EXPECT_TRUE(table.HasKey("beta"));
  EXPECT_EQ("alpha", table.GetString(table.Lookup("alpha")));
}

TEST(RimeStringTableTest, LegacyUnalignedImageOwnsItsData) {
  size_t image_size;
  auto image = MakeImage(&image_size);
  std::vector<ImageBlock> legacy(image.size() + 1);
  ASSERT_LE(image_size, legacy.size() * sizeof(ImageBlock) - 4);
  char* unaligned = reinterpret_cast<char*>(legacy.data()) + 4;
  std::memcpy(unaligned, image.data(), image_size);
  ASSERT_EQ(4U, reinterpret_cast<std::uintptr_t>(unaligned) % 8);
  rime::StringTable table(unaligned, image_size);
  std::memset(unaligned, 0, image_size);
  EXPECT_TRUE(table.HasKey("alpha"));
  EXPECT_TRUE(table.HasKey("beta"));
  EXPECT_EQ("beta", table.GetString(table.Lookup("beta")));
}

TEST(RimeStringTableTest, AllocateBytesAtEightByteBoundary) {
  ByteAllocator file(rime::path{"string_table_alignment_test.bin"});
  file.Remove();
  ASSERT_TRUE(file.Create(4096));
  for (size_t prefix = 1; prefix <= 8; ++prefix) {
    ASSERT_NE(nullptr, file.Allocate<char>(prefix));
    auto image = file.AllocateAlignedBytes(17, 8);
    ASSERT_NE(nullptr, image);
    EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(image) % 8);
  }
  file.Close();
  EXPECT_TRUE(file.Remove());
}

TEST(RimeStringTableTest, NewTableImageIsAligned) {
  rime::Table table(rime::path{"string_table_writer_test.bin"});
  table.Remove();
  rime::Syllabary syllabary{"alpha"};
  rime::Vocabulary vocabulary;
  rime::ShortDictEntry entry;
  entry.text = "alpha";
  entry.code.push_back(0);
  vocabulary[0].entries.push_back(entry);
  ASSERT_TRUE(table.Build(syllabary, vocabulary, 1));
  ASSERT_TRUE(table.Save());
  ASSERT_TRUE(table.Load());
  auto metadata = table.Find<rime::table::Metadata>(0);
  ASSERT_NE(nullptr, metadata);
  EXPECT_EQ(0U,
            reinterpret_cast<std::uintptr_t>(metadata->string_table.get()) % 8);
  EXPECT_EQ("alpha", table.GetSyllableById(0));
  table.Close();
  EXPECT_TRUE(table.Remove());
}

TEST(RimeStringTableTest, NewReverseImagesAreAligned) {
  rime::ReverseDb reverse(rime::path{"string_table_reverse_test.bin"});
  reverse.Remove();
  rime::Syllabary syllabary{"alpha"};
  rime::Vocabulary vocabulary;
  rime::ShortDictEntry entry;
  entry.text = "alpha";
  entry.code.push_back(0);
  vocabulary[0].entries.push_back(entry);
  ASSERT_TRUE(reverse.Build(nullptr, syllabary, vocabulary, {}, 0));
  ASSERT_TRUE(reverse.Save());
  ASSERT_TRUE(reverse.Load());
  auto metadata = reverse.metadata();
  ASSERT_NE(nullptr, metadata);
  EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(metadata->key_trie.get()) % 8);
  EXPECT_EQ(0U,
            reinterpret_cast<std::uintptr_t>(metadata->value_trie.get()) % 8);
  std::string result;
  EXPECT_TRUE(reverse.Lookup("alpha", &result));
  EXPECT_EQ("alpha", result);
  reverse.Close();
  EXPECT_TRUE(reverse.Remove());
}
