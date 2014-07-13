#include <iostream>
#include <string>

#include <marisa.h>

#include "cmdopt.h"

namespace {

bool mmap_flag = true;

void print_help(const char *cmd) {
  std::cerr << "Usage: " << cmd << " [OPTION]... DIC\n\n"
      "Options:\n"
      "  -m, --mmap-dictionary  use memory-mapped I/O to load a dictionary"
      " (default)\n"
      "  -r, --read-dictionary  read an entire dictionary into memory\n"
      "  -h, --help             print this help\n"
      << std::endl;
}

int reverse_lookup(const char * const *args, std::size_t num_args) {
  if (num_args == 0) {
    std::cerr << "error: dictionary is not specified" << std::endl;
    return 10;
  } else if (num_args > 1) {
    std::cerr << "error: more than one dictionaries are specified"
        << std::endl;
    return 11;
  }

  marisa::Trie trie;
  if (mmap_flag) {
    try {
      trie.mmap(args[0]);
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what() << ": failed to mmap a dictionary file: "
          << args[0] << std::endl;
      return 20;
    }
  } else {
    try {
      trie.load(args[0]);
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what() << ": failed to load a dictionary file: "
          << args[0] << std::endl;
      return 21;
    }
  }

  marisa::Agent agent;
  std::size_t key_id;
  while (std::cin >> key_id) {
    try {
      agent.set_query(key_id);
      trie.reverse_lookup(agent);
      std::cout << agent.key().id() << '\t';
      std::cout.write(agent.key().ptr(), agent.key().length()) << '\n';
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what() << ": reverse_lookup() failed: "
          << key_id << std::endl;
      return 30;
    }

    if (!std::cout) {
      std::cerr << "error: failed to write results to standard output"
          << std::endl;
      return 30;
    }
  }

  return 0;
}

}  // namespace

int main(int argc, char *argv[]) {
  std::ios::sync_with_stdio(false);

  ::cmdopt_option long_options[] = {
    { "mmap-dictionary", 0, NULL, 'm' },
    { "read-dictionary", 0, NULL, 'r' },
    { "help", 0, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };
  ::cmdopt_t cmdopt;
  ::cmdopt_init(&cmdopt, argc, argv, "mrh", long_options);
  int label;
  while ((label = ::cmdopt_get(&cmdopt)) != -1) {
    switch (label) {
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
  return reverse_lookup(cmdopt.argv + cmdopt.optind,
      cmdopt.argc - cmdopt.optind);
}
