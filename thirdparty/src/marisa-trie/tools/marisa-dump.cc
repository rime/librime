#ifdef _WIN32
 #include <fcntl.h>
 #include <io.h>
 #include <stdio.h>
#endif  // _WIN32

#include <cstdlib>
#include <iostream>
#include <string>

#include <marisa.h>

#include "cmdopt.h"

namespace {

const char *delimiter = "\n";
bool mmap_flag = true;

void print_help(const char *cmd) {
  std::cerr << "Usage: " << cmd << " [OPTION]... DIC...\n\n"
      "Options:\n"
      "  -d, --delimiter=[S]    specify the delimier (default: \"\\n\")\n"
      "  -m, --mmap-dictionary  use memory-mapped I/O to load a dictionary"
      " (default)\n"
      "  -r, --read-dictionary  read an entire dictionary into memory\n"
      "  -h, --help             print this help\n"
      << std::endl;
}

int dump(const marisa::Trie &trie) {
  std::size_t num_keys = 0;
  marisa::Agent agent;
  agent.set_query("");
  try {
    while (trie.predictive_search(agent)) {
      std::cout.write(agent.key().ptr(), agent.key().length()) << delimiter;
      if (!std::cout) {
        std::cerr << "error: failed to write results to standard output"
            << std::endl;
        return 20;
      }
      ++num_keys;
    }
  } catch (const marisa::Exception &ex) {
    std::cerr << ex.what() << ": predictive_search() failed" << std::endl;
    return 21;
  }
  std::cerr << "#keys: " << num_keys << std::endl;
  return 0;
}

int dump(const char *filename) {
  marisa::Trie trie;
  if (filename != NULL) {
    std::cerr << "input: " << filename << std::endl;
    if (mmap_flag) {
      try {
        trie.mmap(filename);
      } catch (const marisa::Exception &ex) {
        std::cerr << ex.what() << ": failed to mmap a dictionary file: "
            << filename << std::endl;
        return 10;
      }
    } else {
      try {
        trie.load(filename);
      } catch (const marisa::Exception &ex) {
        std::cerr << ex.what() << ": failed to load a dictionary file: "
            << filename << std::endl;
        return 11;
      }
    }
  } else {
    std::cerr << "input: <stdin>" << std::endl;
#ifdef _WIN32
    const int stdin_fileno = ::_fileno(stdin);
    if (stdin_fileno < 0) {
      std::cerr << "error: failed to get the file descriptor of "
          "standard input" << std::endl;
      return 20;
    }
    if (::_setmode(stdin_fileno, _O_BINARY) == -1) {
      std::cerr << "error: failed to set binary mode" << std::endl;
      return 21;
    }
#endif  // _WIN32
    try {
      std::cin >> trie;
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what()
          << ": failed to read a dictionary from standard input" << std::endl;
      return 22;
    }
  }
  return dump(trie);
}

int dump(const char * const *args, std::size_t num_args) {
  if (num_args == 0) {
    return dump(NULL);
  }
  for (std::size_t i = 0; i < num_args; ++i) {
    const int result = dump(args[i]);
    if (result != 0) {
      return result;
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char *argv[]) {
  std::ios::sync_with_stdio(false);

  ::cmdopt_option long_options[] = {
    { "delimiter", 1, NULL, 'd' },
    { "mmap-dictionary", 0, NULL, 'm' },
    { "read-dictionary", 0, NULL, 'r' },
    { "help", 0, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };
  ::cmdopt_t cmdopt;
  ::cmdopt_init(&cmdopt, argc, argv, "d:mrh", long_options);
  int label;
  while ((label = ::cmdopt_get(&cmdopt)) != -1) {
    switch (label) {
      case 'd': {
        delimiter = cmdopt.optarg;
        break;
      }
      case 'm': {
        mmap_flag = true;
        break;
      }
      case 'r': {
        mmap_flag = false;
        break;
      }
      case 'h': {
        print_help(argv[0]);
        return 0;
      }
      default: {
        return 1;
      }
    }
  }
  return dump(cmdopt.argv + cmdopt.optind, cmdopt.argc - cmdopt.optind);
}
