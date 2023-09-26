#ifndef CODEPAGE_H_
#define CODEPAGE_H_

#ifdef _WIN32
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
#else
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = 65001) {
  return 0;
}
#endif /* _WIN32 */

#endif /* CODEPAGE_H_ */
