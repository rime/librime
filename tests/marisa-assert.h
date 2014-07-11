#ifndef MARISA_ASSERT_H_
#define MARISA_ASSERT_H_

#include <iostream>
#include <cstdlib>

#define ASSERT(cond) (void)((!!(cond)) || \
  ((std::cout << __LINE__ << ": Assertion `" << #cond << "' failed." \
      << std::endl), std::exit(-1), 0))

#define EXCEPT(code, expected_error_code) try { \
  code; \
  std::cout << __LINE__ << ": Exception `" << #code << "' failed." \
      << std::endl; \
  std::exit(-1); \
} catch (const marisa::Exception &ex) { \
  ASSERT(ex.error_code() == expected_error_code); \
}

#define TEST_START() \
  (std::cout << __FILE__ << ":" << __LINE__ << ": " << __FUNCTION__ << "(): ")

#define TEST_END() \
  (std::cout << "ok" << std::endl)

#endif  // MARISA_ASSERT_H_
