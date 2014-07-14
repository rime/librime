#ifndef MARISA_CMDOPT_H_
#define MARISA_CMDOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cmdopt_option_ {
  // `name' specifies the name of this option.
  // An array of options must be terminated with an option whose name == NULL.
  const char *name;

  // `has_name' specifies whether an option takes an argument or not.
  // 0 specifies that this option does not have any argument.
  // 1 specifies that this option has an argument.
  // 2 specifies that this option may have an argument.
  int  has_arg;

  // `flag' specifies an integer variable which is overwritten by cmdopt_next()
  // with its return value.
  int *flag;

  // `val' specifies a return value of cmdopt_next(). This value is returned
  // when cmdopt_next() finds this option.
  int  val;
} cmdopt_option;

typedef struct cmdopt_t_ {
  // Command line arguments.
  int    argc;
  char **argv;

  // Option settings.
  const cmdopt_option *longopts;
  const char          *optstring;

  int   optind;     // Index of the next argument.
  char *nextchar;   // Next character.
  char *optarg;     // Argument of the last option.
  int   optopt;     // Label of the last option.
  char *optlong;    // Long option.
  int   opterr;     // Warning level (0: nothing, 1: warning, 2: all).
  int   longindex;  // Index of the last long option.
  int   optnum;     // Number of options.
} cmdopt_t;

// cmdopt_init() initializes a cmdopt_t for successive cmdopt_next()s.
void cmdopt_init(cmdopt_t *h, int argc, char **argv,
    const char *optstring, const cmdopt_option *longopts);

// cmdopt_get() analyzes command line arguments and gets the next option.
int cmdopt_get(cmdopt_t *h);

#ifdef  __cplusplus
}  // extern "C"
#endif

#endif  // MARISA_CMDOPT_H_
