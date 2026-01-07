#ifndef RIME_API_STDBOOL_H_
#define RIME_API_STDBOOL_H_

#include <stdbool.h>

#undef RIME_FLAVORED
#define RIME_FLAVORED(name) name##_stdbool

// do not export librime 0.9 API in this build variant
#undef RIME_DEPRECATED
#define RIME_DEPRECATED static

#undef Bool
#define Bool bool

#undef False
#define False false

#undef True
#define True true

#endif  // RIME_API_STDBOOL_H_
