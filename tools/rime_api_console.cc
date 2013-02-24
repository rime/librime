/*
 * Copyleft 2011 RIME Developers
 * License: GPLv3
 *
 * 2011-08-29 GONG Chen <chen.sst@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <rime_api.h>

void PrintStatus(RimeStatus *status) {
  printf("schema: %s / %s\n",
         status->schema_id, status->schema_name);
  printf("status: ");
  if (status->is_disabled) printf("disabled ");
  if (status->is_composing) printf("composing ");
  if (status->is_ascii_mode) printf("ascii ");
  if (status->is_full_shape) printf("full_shape ");
  if (status->is_simplified) printf("simplified ");
  if (status->is_traditionalized) printf("traditionalized ");
  printf("\n");
}

void PrintComposition(RimeComposition *composition) {
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

void PrintMenu(RimeMenu *menu) {
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

void PrintContext(RimeContext *context) {
  if (context->composition.length > 0) {
    PrintComposition(&context->composition);
    PrintMenu(&context->menu);
  }
  else {
    printf("(not composing)\n");
  }
}

void Print(RimeSessionId session_id) {
  RimeCommit commit = {0};
  RimeStatus status = {0};
  RimeContext context = {0};
  RIME_STRUCT_INIT(RimeStatus, status);
  RIME_STRUCT_INIT(RimeContext, context);

  if (RimeGetCommit(session_id, &commit)) {
    printf("commit: %s\n", commit.text);
    RimeFreeCommit(&commit);
  }

  if (RimeGetStatus(session_id, &status)) {
    PrintStatus(&status);
    RimeFreeStatus(&status);
  }

  if (RimeGetContext(session_id, &context)) {
    PrintContext(&context);
    RimeFreeContext(&context);
  }
}

bool ExecuteSpecialCommand(const char* line, RimeSessionId session_id) {
    if (!strcmp(line, "print schema list")) {
      RimeSchemaList list;
      if (RimeGetSchemaList(&list)) {
        printf("schema list:\n");
        for (size_t i = 0; i < list.size; ++i) {
          printf("%ld. %s [%s]\n", (i + 1),
                 list.list[i].name, list.list[i].schema_id);
        }
        RimeFreeSchemaList(&list);
      }
      char current[100] = {0};
      if (RimeGetCurrentSchema(session_id, current, sizeof(current))) {
        printf("current schema: [%s]\n", current);
      }
      return true;
    }
    if (!strncmp(line, "select schema ", 14)) {
      const char* schema_id = line + 14;
      if (RimeSelectSchema(session_id, schema_id)) {
        printf("selected schema: [%s]\n", schema_id);
      }
      return true;
    }
    return false;
}

void OnMessage(void* context_object,
               RimeSessionId session_id,
               const char* message_type,
               const char* message_value) {
  printf("message: [%x] [%s] %s\n", session_id, message_type, message_value);
}

int main(int argc, char *argv[]) {
  RimeSetupLogging("rime.console");
  RimeSetNotificationHandler(&OnMessage, NULL);

  fprintf(stderr, "initializing...\n");
  RimeInitialize(NULL);
  if (RimeStartMaintenance(True))
    RimeJoinMaintenanceThread();
  fprintf(stderr, "ready.\n");

  RimeSessionId session_id = RimeCreateSession();
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
    if (ExecuteSpecialCommand(line, session_id))
      continue;
    if (!RimeSimulateKeySequence(session_id, line)) {
      fprintf(stderr, "Error processing key sequence: %s\n", line);
    }
    else {
      Print(session_id);
    }
  }

  RimeDestroySession(session_id);

  RimeFinalize();

  return 0;
}
