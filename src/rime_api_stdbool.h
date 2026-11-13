#ifndef RIME_API_STDBOOL_H_
#define RIME_API_STDBOOL_H_

#include <stdbool.h>

#undef RIME_FLAVORED
#define RIME_FLAVORED(name) name##_stdbool

// do not export librime 0.9 API in this build variant
#undef RIME_DEPRECATED
#ifdef __cplusplus
#define RIME_DEPRECATED [[maybe_unused]] static
#else
#define RIME_DEPRECATED static
#endif

#undef Bool
#define Bool bool

#undef False
#define False false

#undef True
#define True true

#endif  // RIME_API_STDBOOL_H_
