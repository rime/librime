#include <rime_api_stdbool.h>
#include <rime_levers_api.h>
// defines rime_levers_get_api_stdbool()
#include "levers_api_impl.h"

static void rime_levers_stdbool_initialize() {}
static void rime_levers_stdbool_finalize() {}

RIME_REGISTER_CUSTOM_MODULE(levers_stdbool) {
  module->get_api = rime_levers_get_api_stdbool;
}
