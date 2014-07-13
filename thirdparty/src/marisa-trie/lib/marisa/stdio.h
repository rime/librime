#ifndef MARISA_STDIO_H_
#define MARISA_STDIO_H_

#include <cstdio>

namespace marisa {

class Trie;

void fread(std::FILE *file, Trie *trie);
void fwrite(std::FILE *file, const Trie &trie);

}  // namespace marisa

#endif  // MARISA_STDIO_H_
