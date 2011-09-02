/* vim: set sts=2 sw=2 et:
 * encoding: utf-8
 *
 * Copyleft 2011 RIME Developers
 * License: GPLv3
 * 
 * 2011-08-29 GONG Chen <chen.sst@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <rime_api.h>

int min(int x, int y) {
  return x > y ? y : x;
}

void PrintStatus(RimeStatus *status) {
  printf("schema: %s / %s\n",
         status->schema_id, status->schema_name);
  printf("status: ");
  if (status->is_disabled) printf("disabled ");
  if (status->is_ascii_mode) printf("ascii ");
  if (status->is_simplified) printf("simplified ");
  printf("\n");
}

void PrintComposition(RimeComposition *composition) {
  const char *preedit = composition->preedit;
  if (!preedit) return;
  int len = strlen(preedit);
  int start = min(len, composition->sel_start);
  int end = min(len, composition->sel_end);
  int cursor = min(len, composition->cursor_pos);
  for (int i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) putchar('[');
      else if (i == end) putchar(']');
    }
    if (i == cursor) putchar('|');
    if (i < len) putchar(preedit[i]);
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
    printf("%d. %c%s%c\n",
           i + 1,
           highlighted ? '<' : ' ',
           menu->candidates[i],
           highlighted ? '>' : ' ');
  }
}

void PrintContext(RimeContext *context) {
  if (context->composition.is_composing) {
    PrintComposition(&context->composition);
    PrintMenu(&context->menu);
  }
  else {
    printf("(not composing)\n");
  }
}

void Print(RimeSessionId session_id) {
  RimeCommit commit;
  RimeStatus status;
  RimeContext context;

  if (RimeGetCommit(session_id, &commit)) {
    printf("commit: %s\n", commit.text);
  }

  if (RimeGetStatus(session_id, &status))
    PrintStatus(&status);
  
  if (RimeGetContext(session_id, &context))
    PrintContext(&context);
}

int main(int argc, char *argv[]) {

  RimeInitialize();

  RimeSessionId session_id = RimeCreateSession();
  if (!session_id) {
    fprintf(stderr, "Error creating rime session.\n");
    return 1;
  }

  char line[kRimeTextMaxLength + 1];
  while (fgets(line, kRimeTextMaxLength, stdin) != NULL) {
    for (char *p = line; *p; ++p) {
      if (*p == '\r' || *p == '\n') {
        *p = '\0';
        break;
      }
    }
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
