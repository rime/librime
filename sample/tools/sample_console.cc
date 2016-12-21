/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2014-01-04 GONG Chen <chen.sst@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <rime_api.h>

void print_status(RimeStatus *status) {
  printf("schema: %s / %s\n",
         status->schema_id, status->schema_name);
  printf("status: ");
  if (status->is_disabled) printf("disabled ");
  if (status->is_composing) printf("composing ");
  if (status->is_ascii_mode) printf("ascii ");
  if (status->is_full_shape) printf("full_shape ");
  if (status->is_simplified) printf("simplified ");
  printf("\n");
}

void print_composition(RimeComposition *composition) {
  const char *preedit = composition->preedit;
  if (!preedit) return;
  size_t len = strlen(preedit);
  size_t start = composition->sel_start;
  size_t end = composition->sel_end;
  //size_t cursor = composition->cursor_pos;
  for (size_t i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) putchar('[');
      else if (i == end) putchar(']');
    }
    //if (i == cursor) putchar('|');
    if (i < len)
        putchar(preedit[i]);
  }
  printf("\n");
}

void print_menu(RimeMenu *menu) {
  if (menu->num_candidates == 0) return;
  printf("page: %d%c (of size %d)\n",
         menu->page_no + 1,
         menu->is_last_page ? '$' : ' ',
         menu->page_size);
  for (int i = 0; i < menu->num_candidates; ++i) {
    bool highlighted = i == menu->highlighted_candidate_index;
    printf("%d. %c%s%c%s\n",
           i + 1,
           highlighted ? '[' : ' ',
           menu->candidates[i].text,
           highlighted ? ']' : ' ',
           menu->candidates[i].comment ? menu->candidates[i].comment : "");
  }
}

void print_context(RimeContext *context) {
  if (context->composition.length > 0) {
    print_composition(&context->composition);
    print_menu(&context->menu);
  }
  else {
    printf("(not composing)\n");
  }
}

void print(RimeSessionId session_id) {
  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeCommit, commit);
  RIME_STRUCT(RimeStatus, status);
  RIME_STRUCT(RimeContext, context);

  if (rime->get_commit(session_id, &commit)) {
    printf("commit: %s\n", commit.text);
    rime->free_commit(&commit);
  }

  if (rime->get_status(session_id, &status)) {
    print_status(&status);
    rime->free_status(&status);
  }

  if (rime->get_context(session_id, &context)) {
    print_context(&context);
    rime->free_context(&context);
  }
}

bool execute_special_command(const char* line, RimeSessionId session_id) {
  RimeApi* rime = rime_get_api();
  if (!strcmp(line, "print schema list")) {
    RimeSchemaList list;
    if (rime->get_schema_list(&list)) {
      printf("schema list:\n");
      for (size_t i = 0; i < list.size; ++i) {
        printf("%lu. %s [%s]\n", (i + 1),
               list.list[i].name, list.list[i].schema_id);
      }
      rime->free_schema_list(&list);
    }
    char current[100] = {0};
    if (rime->get_current_schema(session_id, current, sizeof(current))) {
      printf("current schema: [%s]\n", current);
    }
    return true;
  }
  if (!strncmp(line, "select schema ", 14)) {
    const char* schema_id = line + 14;
    if (rime->select_schema(session_id, schema_id)) {
      printf("selected schema: [%s]\n", schema_id);
    }
    return true;
  }
  return false;
}

void on_message(void* context_object,
                RimeSessionId session_id,
                const char* message_type,
                const char* message_value) {
  printf("message: [%lu] [%s] %s\n", session_id, message_type, message_value);
}

static RIME_MODULE_LIST(sample_modules, "default", "sample");

int main(int argc, char *argv[]) {
  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "sample.console";
  traits.modules = sample_modules;
  rime->setup(&traits);

  rime->set_notification_handler(&on_message, NULL);

  fprintf(stderr, "initializing...\n");
  rime->initialize(&traits);
  Bool full_check = True;
  if (rime->start_maintenance(full_check))
    rime->join_maintenance_thread();
  fprintf(stderr, "ready.\n");

  RimeSessionId session_id = rime->create_session();
  if (!session_id) {
    fprintf(stderr, "Error creating rime session.\n");
    return 1;
  }

  const int kMaxLength = 99;
  char line[kMaxLength + 1] = {0};
  while (fgets(line, kMaxLength, stdin) != NULL) {
    for (char *p = line; *p; ++p) {
      if (*p == '\r' || *p == '\n') {
        *p = '\0';
        break;
      }
    }
    if (!strcmp(line, "exit"))
      break;
    if (execute_special_command(line, session_id))
      continue;
    if (!rime->simulate_key_sequence(session_id, line)) {
      fprintf(stderr, "Error processing key sequence: %s\n", line);
    }
    else {
      print(session_id);
    }
  }

  rime->destroy_session(session_id);

  rime->finalize();

  return 0;
}
