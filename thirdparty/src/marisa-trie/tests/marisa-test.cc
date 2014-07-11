#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>

#include <marisa.h>

#include "marisa-assert.h"

namespace {

void TestEmptyTrie() {
  TEST_START();

  marisa::Trie trie;

  EXCEPT(trie.save("marisa-test.dat"), MARISA_STATE_ERROR);
#ifdef _MSC_VER
  EXCEPT(trie.write(::_fileno(stdout)), MARISA_STATE_ERROR);
#else  // _MSC_VER
  EXCEPT(trie.write(::fileno(stdout)), MARISA_STATE_ERROR);
#endif  // _MSC_VER
  EXCEPT(std::cout << trie, MARISA_STATE_ERROR);
  EXCEPT(marisa::fwrite(stdout, trie), MARISA_STATE_ERROR);

  marisa::Agent agent;

  EXCEPT(trie.lookup(agent), MARISA_STATE_ERROR);
  EXCEPT(trie.reverse_lookup(agent), MARISA_STATE_ERROR);
  EXCEPT(trie.common_prefix_search(agent), MARISA_STATE_ERROR);
  EXCEPT(trie.predictive_search(agent), MARISA_STATE_ERROR);

  EXCEPT(trie.num_tries(), MARISA_STATE_ERROR);
  EXCEPT(trie.num_keys(), MARISA_STATE_ERROR);
  EXCEPT(trie.num_nodes(), MARISA_STATE_ERROR);

  EXCEPT(trie.tail_mode(), MARISA_STATE_ERROR);
  EXCEPT(trie.node_order(), MARISA_STATE_ERROR);

  EXCEPT(trie.empty(), MARISA_STATE_ERROR);
  EXCEPT(trie.size(), MARISA_STATE_ERROR);
  EXCEPT(trie.total_size(), MARISA_STATE_ERROR);
  EXCEPT(trie.io_size(), MARISA_STATE_ERROR);

  marisa::Keyset keyset;
  trie.build(keyset);

  ASSERT(!trie.lookup(agent));
  EXCEPT(trie.reverse_lookup(agent), MARISA_BOUND_ERROR);
  ASSERT(!trie.common_prefix_search(agent));
  ASSERT(!trie.predictive_search(agent));

  ASSERT(trie.num_tries() == 1);
  ASSERT(trie.num_keys() == 0);
  ASSERT(trie.num_nodes() == 1);

  ASSERT(trie.tail_mode() == MARISA_DEFAULT_TAIL);
  ASSERT(trie.node_order() == MARISA_DEFAULT_ORDER);

  ASSERT(trie.empty());
  ASSERT(trie.size() == 0);
  ASSERT(trie.total_size() != 0);
  ASSERT(trie.io_size() != 0);

  keyset.push_back("");
  trie.build(keyset);

  ASSERT(trie.lookup(agent));
  trie.reverse_lookup(agent);
  ASSERT(trie.common_prefix_search(agent));
  ASSERT(!trie.common_prefix_search(agent));
  ASSERT(trie.predictive_search(agent));
  ASSERT(!trie.predictive_search(agent));

  ASSERT(trie.num_keys() == 1);
  ASSERT(trie.num_nodes() == 1);

  ASSERT(!trie.empty());
  ASSERT(trie.size() == 1);
  ASSERT(trie.total_size() != 0);
  ASSERT(trie.io_size() != 0);

  TEST_END();
}

void TestTinyTrie() {
  TEST_START();

  marisa::Keyset keyset;
  keyset.push_back("bach");
  keyset.push_back("bet");
  keyset.push_back("chat");
  keyset.push_back("check");
  keyset.push_back("check");

  marisa::Trie trie;
  trie.build(keyset, 1);

  ASSERT(trie.num_tries() == 1);
  ASSERT(trie.num_keys() == 4);
  ASSERT(trie.num_nodes() == 7);

  ASSERT(trie.tail_mode() == MARISA_DEFAULT_TAIL);
  ASSERT(trie.node_order() == MARISA_DEFAULT_ORDER);

  ASSERT(keyset[0].id() == 2);
  ASSERT(keyset[1].id() == 3);
  ASSERT(keyset[2].id() == 1);
  ASSERT(keyset[3].id() == 0);
  ASSERT(keyset[4].id() == 0);

  marisa::Agent agent;
  for (std::size_t i = 0; i < keyset.size(); ++i) {
    agent.set_query(keyset[i].ptr(), keyset[i].length());
    ASSERT(trie.lookup(agent));
    ASSERT(agent.key().id() == keyset[i].id());

    agent.set_query(keyset[i].id());
    trie.reverse_lookup(agent);
    ASSERT(agent.key().length() == keyset[i].length());
    ASSERT(std::memcmp(agent.key().ptr(), keyset[i].ptr(),
        agent.key().length()) == 0);
  }

  agent.set_query("be");
  ASSERT(!trie.common_prefix_search(agent));
  agent.set_query("beX");
  ASSERT(!trie.common_prefix_search(agent));
  agent.set_query("bet");
  ASSERT(trie.common_prefix_search(agent));
  ASSERT(!trie.common_prefix_search(agent));
  agent.set_query("betX");
  ASSERT(trie.common_prefix_search(agent));
  ASSERT(!trie.common_prefix_search(agent));

  agent.set_query("chatX");
  ASSERT(!trie.predictive_search(agent));
  agent.set_query("chat");
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 4);
  ASSERT(!trie.predictive_search(agent));

  agent.set_query("cha");
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 4);
  ASSERT(!trie.predictive_search(agent));

  agent.set_query("c");
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 5);
  ASSERT(std::memcmp(agent.key().ptr(), "check", 5) == 0);
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 4);
  ASSERT(std::memcmp(agent.key().ptr(), "chat", 4) == 0);
  ASSERT(!trie.predictive_search(agent));

  agent.set_query("ch");
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 5);
  ASSERT(std::memcmp(agent.key().ptr(), "check", 5) == 0);
  ASSERT(trie.predictive_search(agent));
  ASSERT(agent.key().length() == 4);
  ASSERT(std::memcmp(agent.key().ptr(), "chat", 4) == 0);
  ASSERT(!trie.predictive_search(agent));

  trie.build(keyset, 1 | MARISA_LABEL_ORDER);

  ASSERT(trie.num_tries() == 1);
  ASSERT(trie.num_keys() == 4);
  ASSERT(trie.num_nodes() == 7);

  ASSERT(trie.tail_mode() == MARISA_DEFAULT_TAIL);
  ASSERT(trie.node_order() == MARISA_LABEL_ORDER);

  ASSERT(keyset[0].id() == 0);
  ASSERT(keyset[1].id() == 1);
  ASSERT(keyset[2].id() == 2);
  ASSERT(keyset[3].id() == 3);
  ASSERT(keyset[4].id() == 3);

  for (std::size_t i = 0; i < keyset.size(); ++i) {
    agent.set_query(keyset[i].ptr(), keyset[i].length());
    ASSERT(trie.lookup(agent));
    ASSERT(agent.key().id() == keyset[i].id());

    agent.set_query(keyset[i].id());
    trie.reverse_lookup(agent);
    ASSERT(agent.key().length() == keyset[i].length());
    ASSERT(std::memcmp(agent.key().ptr(), keyset[i].ptr(),
        agent.key().length()) == 0);
  }

  agent.set_query("");
  for (std::size_t i = 0; i < trie.size(); ++i) {
    ASSERT(trie.predictive_search(agent));
    ASSERT(agent.key().id() == i);
  }
  ASSERT(!trie.predictive_search(agent));

  TEST_END();
}

void MakeKeyset(std::size_t num_keys, marisa::TailMode tail_mode,
    marisa::Keyset *keyset) {
  char key_buf[16];
  for (std::size_t i = 0; i < num_keys; ++i) {
    const std::size_t length = std::rand() % sizeof(key_buf);
    for (std::size_t j = 0; j < length; ++j) {
      key_buf[j] = (char)(std::rand() % 10);
      if (tail_mode == MARISA_TEXT_TAIL) {
        key_buf[j] += '0';
      }
    }
    keyset->push_back(key_buf, length);
  }
}

void TestLookup(const marisa::Trie &trie, const marisa::Keyset &keyset) {
  marisa::Agent agent;
  for (std::size_t i = 0; i < keyset.size(); ++i) {
    agent.set_query(keyset[i].ptr(), keyset[i].length());
    ASSERT(trie.lookup(agent));
    ASSERT(agent.key().id() == keyset[i].id());

    agent.set_query(keyset[i].id());
    trie.reverse_lookup(agent);
    ASSERT(agent.key().length() == keyset[i].length());
    ASSERT(std::memcmp(agent.key().ptr(), keyset[i].ptr(),
        agent.key().length()) == 0);
  }
}

void TestCommonPrefixSearch(const marisa::Trie &trie,
    const marisa::Keyset &keyset) {
  marisa::Agent agent;
  for (std::size_t i = 0; i < keyset.size(); ++i) {
    agent.set_query(keyset[i].ptr(), keyset[i].length());
    ASSERT(trie.common_prefix_search(agent));
    ASSERT(agent.key().id() <= keyset[i].id());
    while (trie.common_prefix_search(agent)) {
      ASSERT(agent.key().id() <= keyset[i].id());
    }
    ASSERT(agent.key().id() == keyset[i].id());
  }
}

void TestPredictiveSearch(const marisa::Trie &trie,
    const marisa::Keyset &keyset) {
  marisa::Agent agent;
  for (std::size_t i = 0; i < keyset.size(); ++i) {
    agent.set_query(keyset[i].ptr(), keyset[i].length());
    ASSERT(trie.predictive_search(agent));
    ASSERT(agent.key().id() == keyset[i].id());
    while (trie.predictive_search(agent)) {
      ASSERT(agent.key().id() > keyset[i].id());
    }
  }
}

void TestTrie(int num_tries, marisa::TailMode tail_mode,
    marisa::NodeOrder node_order, marisa::Keyset &keyset) {
  for (std::size_t i = 0; i < keyset.size(); ++i) {
    keyset[i].set_weight(1.0F);
  }

  marisa::Trie trie;
  trie.build(keyset, num_tries | tail_mode | node_order);

  ASSERT(trie.num_tries() == (std::size_t)num_tries);
  ASSERT(trie.num_keys() <= keyset.size());

  ASSERT(trie.tail_mode() == tail_mode);
  ASSERT(trie.node_order() == node_order);

  TestLookup(trie, keyset);
  TestCommonPrefixSearch(trie, keyset);
  TestPredictiveSearch(trie, keyset);

  trie.save("marisa-test.dat");

  trie.clear();
  trie.load("marisa-test.dat");

  ASSERT(trie.num_tries() == (std::size_t)num_tries);
  ASSERT(trie.num_keys() <= keyset.size());

  ASSERT(trie.tail_mode() == tail_mode);
  ASSERT(trie.node_order() == node_order);

  TestLookup(trie, keyset);

  {
    std::FILE *file;
#ifdef _MSC_VER
    ASSERT(::fopen_s(&file, "marisa-test.dat", "wb") == 0);
#else  // _MSC_VER
    file = std::fopen("marisa-test.dat", "wb");
    ASSERT(file != NULL);
#endif  // _MSC_VER
    marisa::fwrite(file, trie);
    std::fclose(file);
    trie.clear();
#ifdef _MSC_VER
    ASSERT(::fopen_s(&file, "marisa-test.dat", "rb") == 0);
#else  // _MSC_VER
    file = std::fopen("marisa-test.dat", "rb");
    ASSERT(file != NULL);
#endif  // _MSC_VER
    marisa::fread(file, &trie);
    std::fclose(file);
  }

  ASSERT(trie.num_tries() == (std::size_t)num_tries);
  ASSERT(trie.num_keys() <= keyset.size());

  ASSERT(trie.tail_mode() == tail_mode);
  ASSERT(trie.node_order() == node_order);

  TestLookup(trie, keyset);

  trie.clear();
  trie.mmap("marisa-test.dat");

  ASSERT(trie.num_tries() == (std::size_t)num_tries);
  ASSERT(trie.num_keys() <= keyset.size());

  ASSERT(trie.tail_mode() == tail_mode);
  ASSERT(trie.node_order() == node_order);

  TestLookup(trie, keyset);

  {
    std::stringstream stream;
    stream << trie;
    trie.clear();
    stream >> trie;
  }

  ASSERT(trie.num_tries() == (std::size_t)num_tries);
  ASSERT(trie.num_keys() <= keyset.size());

  ASSERT(trie.tail_mode() == tail_mode);
  ASSERT(trie.node_order() == node_order);

  TestLookup(trie, keyset);
}

void TestTrie(marisa::TailMode tail_mode, marisa::NodeOrder node_order,
    marisa::Keyset &keyset) {
  TEST_START();
  std::cout << ((tail_mode == MARISA_TEXT_TAIL) ? "TEXT" : "BINARY") << ", ";
  std::cout << ((node_order == MARISA_WEIGHT_ORDER) ?
      "WEIGHT" : "LABEL") << ": ";

  for (int i = 1; i < 5; ++i) {
    TestTrie(i, tail_mode, node_order, keyset);
  }

  TEST_END();
}

void TestTrie(marisa::TailMode tail_mode) {
  marisa::Keyset keyset;
  MakeKeyset(1000, tail_mode, &keyset);

  TestTrie(tail_mode, MARISA_WEIGHT_ORDER, keyset);
  TestTrie(tail_mode, MARISA_LABEL_ORDER, keyset);
}

void TestTrie() {
  TestTrie(MARISA_TEXT_TAIL);
  TestTrie(MARISA_BINARY_TAIL);
}

}  // namespace

int main() try {
  std::srand((unsigned int)std::time(NULL));

  TestEmptyTrie();
  TestTinyTrie();
  TestTrie();

  return 0;
} catch (const marisa::Exception &ex) {
  std::cerr << ex.what() << std::endl;
  throw;
}
