/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2011-08-29 GONG Chen <chen.sst@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rime_api.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "codepage.h"
#include "line_editor.h"

namespace {

const char* const kConsoleCommands[] = {
    "reload",
    "exit",
    "print candidate list",
    "print schema list",
    "select candidate ",
    "select schema ",
    "set option ",
    "delete ",
    "delete on current page ",
    "synchronize",
};

const char* const kKeyNames[] = {
    "Escape",
    "Return",
    "space",
    "Tab",
    "BackSpace",
    "Left",
    "Right",
    "Up",
    "Down",
    "Home",
    "End",
    "Page_Up",
    "Page_Down",
    "Prior",
    "Next",
    "Begin",
    "Delete",
    "Insert",
    "Clear",
    "Linefeed",
    "Pause",
    "Scroll_Lock",
    "Sys_Req",
    "Print",
    "Execute",
    "Select",
    "Undo",
    "Redo",
    "Menu",
    "Find",
    "Cancel",
    "Help",
    "Break",
    "Num_Lock",
    "Caps_Lock",
    "KP_Equal",
    "Shift_L",
    "Shift_R",
    "Control_L",
    "Control_R",
    "Alt_L",
    "Alt_R",
    "Super_L",
    "Super_R",
    "Meta_L",
    "Meta_R",
    "Hyper_L",
    "Hyper_R",
    "Multi_key",
    "Mode_switch",
    "ISO_Level3_Shift",
    "ISO_Level3_Latch",
    "ISO_Level3_Lock",
    "grave",
    "minus",
    "equal",
    "plus",
    "underscore",
    "parenleft",
    "parenright",
    "bracketleft",
    "bracketright",
    "braceleft",
    "braceright",
    "less",
    "greater",
    "colon",
    "semicolon",
    "comma",
    "period",
    "slash",
    "backslash",
    "bar",
    "question",
    "exclam",
    "quotedbl",
    "apostrophe",
    "quoteleft",
    "quoteright",
    "numbersign",
    "dollar",
    "percent",
    "ampersand",
    "asterisk",
    "at",
    "asciitilde",
    "asciicircum",
    "KP_0",
    "KP_1",
    "KP_2",
    "KP_3",
    "KP_4",
    "KP_5",
    "KP_6",
    "KP_7",
    "KP_8",
    "KP_9",
    "KP_Add",
    "KP_Subtract",
    "KP_Multiply",
    "KP_Divide",
    "KP_Decimal",
    "KP_Separator",
    "KP_Enter",
    "KP_Space",
    "KP_Tab",
    "KP_Home",
    "KP_Left",
    "KP_Up",
    "KP_Right",
    "KP_Down",
    "KP_Page_Up",
    "KP_Page_Down",
    "KP_End",
    "KP_Begin",
    "KP_Insert",
    "KP_Delete",
    "Shift+",
    "Control+",
    "Alt+",
    "Super+",
    "Lock+",
    "Release+",
};

const char* const kComboKeys[] = {
    "grave",      "space",      "Tab",         "Return",       "BackSpace",
    "Escape",     "Left",       "Right",       "Up",           "Down",
    "Home",       "End",        "Page_Up",     "Page_Down",    "Delete",
    "Insert",     "minus",      "equal",       "plus",         "underscore",
    "parenleft",  "parenright", "bracketleft", "bracketright", "braceleft",
    "braceright", "less",       "greater",     "colon",        "semicolon",
    "comma",      "period",     "slash",       "backslash",    "bar",
    "question",   "exclam",     "quotedbl",    "apostrophe",   "quoteleft",
    "quoteright", "numbersign", "dollar",      "percent",      "ampersand",
    "asterisk",   "at",         "asciitilde",  "asciicircum",
};

const char* const kModifierOrders[] = {
    "Shift",
    "Control",
    "Alt",
    "Super",
    "Shift+Control",
    "Control+Shift",
    "Shift+Alt",
    "Alt+Shift",
    "Control+Alt",
    "Alt+Control",
    "Shift+Super",
    "Super+Shift",
    "Control+Super",
    "Super+Control",
    "Alt+Super",
    "Super+Alt",
    "Control+Shift+Alt",
    "Control+Alt+Shift",
    "Shift+Control+Alt",
    "Shift+Alt+Control",
    "Alt+Control+Shift",
    "Alt+Shift+Control",
    "Control+Shift+Super",
    "Control+Super+Shift",
    "Shift+Control+Super",
    "Shift+Super+Control",
    "Super+Control+Shift",
    "Super+Shift+Control",
    "Control+Shift+Alt+Super",
};

class CompletionCatalog {
 public:
  CompletionCatalog() {
    for (const char* command : kConsoleCommands)
      AddCommand(command);
    for (const char* key : kKeyNames)
      AddKey(key);
    for (int i = 1; i <= 12; ++i)
      AddKey("F" + std::to_string(i));
    for (char c = 'a'; c <= 'z'; ++c)
      AddKey(std::string(1, c));
    for (char c = '0'; c <= '9'; ++c)
      AddKey(std::string(1, c));

    std::vector<std::string> combo_keys(
        kComboKeys, kComboKeys + sizeof(kComboKeys) / sizeof(kComboKeys[0]));
    for (int i = 1; i <= 12; ++i)
      combo_keys.push_back("F" + std::to_string(i));
    for (char c = 'a'; c <= 'z'; ++c)
      combo_keys.push_back(std::string(1, c));
    for (char c = '0'; c <= '9'; ++c)
      combo_keys.push_back(std::string(1, c));

    for (const char* modifiers : kModifierOrders) {
      for (const std::string& key : combo_keys)
        AddKey(std::string(modifiers) + "+" + key);
    }
  }

  const std::vector<LineCompletion>& items() const { return items_; }

 private:
  void AddCommand(const std::string& command) {
    Add(command, command, "console command", "command");
  }

  void AddKey(const std::string& key) {
    Add("{" + key + "}", "{" + key + "}", "", "key");
  }

  void Add(const std::string& text,
           const std::string& display,
           const std::string& detail,
           const std::string& kind) {
    if (!seen_.insert(text).second)
      return;
    LineCompletion item;
    item.text = text;
    item.display = display;
    item.detail = detail;
    item.kind = kind;
    items_.push_back(item);
  }

  std::vector<LineCompletion> items_;
  std::unordered_set<std::string> seen_;
};

std::string FormatStatusHeader(const RimeStatus& status) {
  std::string header = "schema: ";
  header += status.schema_id ? status.schema_id : "?";
  header += " / ";
  header += status.schema_name ? status.schema_name : "?";
  header += "  [";

  bool first = true;
  const auto add_flag = [&](const char* name, Bool enabled) {
    if (!enabled)
      return;
    if (!first)
      header += ' ';
    header += name;
    first = false;
  };
  add_flag("disabled", status.is_disabled);
  add_flag("composing", status.is_composing);
  add_flag("ascii_mode", status.is_ascii_mode);
  add_flag("full_shape", status.is_full_shape);
  add_flag("simplified", status.is_simplified);
  if (first)
    header += "ready";
  header += ']';
  return header;
}

std::string MakeStatusHeader(RimeApi* rime, RimeSessionId session_id) {
  if (!session_id)
    return std::string();
  RIME_STRUCT(RimeStatus, status);
  if (!rime->get_status(session_id, &status))
    return std::string();
  std::string header = FormatStatusHeader(status);
  rime->free_status(&status);
  return header;
}

void AppendSchemaCompletions(RimeApi* rime,
                             std::vector<LineCompletion>* completions) {
  RimeSchemaList list = {0};
  if (!rime->get_schema_list(&list))
    return;
  for (size_t i = 0; i < list.size; ++i) {
    const RimeSchemaListItem& schema = list.list[i];
    if (!schema.schema_id)
      continue;
    LineCompletion item;
    item.text = std::string("select schema ") + schema.schema_id;
    item.display = schema.schema_id;
    item.detail = schema.name ? schema.name : "";
    item.kind = "schema";
    completions->push_back(item);
  }
  rime->free_schema_list(&list);
}

void AddOptionCompletion(const std::string& option,
                         std::vector<LineCompletion>* completions) {
  LineCompletion item;
  item.text = "set option " + option;
  item.display = option;
  item.detail = "schema option";
  item.kind = "option";
  completions->push_back(item);
}

void AppendOptionCompletions(RimeApi* rime,
                             const char* schema_id,
                             std::vector<LineCompletion>* completions) {
  RimeConfig config = {0};
  if (!rime->schema_open(schema_id, &config))
    return;

  RimeConfigIterator iterator = {0};
  if (rime->config_begin_list(&iterator, &config, "switches")) {
    while (rime->config_next(&iterator)) {
      std::string item_path = iterator.path ? iterator.path : "";
      const char* name =
          rime->config_get_cstring(&config, (item_path + "/name").c_str());
      if (name) {
        AddOptionCompletion(name, completions);
        continue;
      }
      RimeConfigIterator option_iterator = {0};
      if (rime->config_begin_list(&option_iterator, &config,
                                  (item_path + "/options").c_str())) {
        while (rime->config_next(&option_iterator)) {
          const char* option =
              rime->config_get_cstring(&config, option_iterator.path);
          if (option)
            AddOptionCompletion(option, completions);
        }
        rime->config_end(&option_iterator);
      }
    }
    rime->config_end(&iterator);
  }
  rime->config_close(&config);
}

std::vector<LineCompletion> BuildCompletions(RimeApi* rime,
                                             RimeSessionId session_id) {
  static const CompletionCatalog catalog;
  std::vector<LineCompletion> completions = catalog.items();
  AppendSchemaCompletions(rime, &completions);
  RIME_STRUCT(RimeStatus, status);
  if (rime->get_status(session_id, &status)) {
    if (status.schema_id)
      AppendOptionCompletions(rime, status.schema_id, &completions);
    rime->free_status(&status);
  }
  return completions;
}

}  // namespace

void print_status(RimeStatus* status) {
  std::string header = FormatStatusHeader(*status);
  printf("%s\n", header.c_str());
}

void print_composition(RimeComposition* composition) {
  const char* preedit = composition->preedit;
  if (!preedit)
    return;
  size_t len = strlen(preedit);
  size_t start = composition->sel_start;
  size_t end = composition->sel_end;
  size_t cursor = composition->cursor_pos;
  for (size_t i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) {
        putchar('[');
      } else if (i == end) {
        putchar(']');
      }
    }
    if (i == cursor)
      putchar('|');
    if (i < len)
      putchar(preedit[i]);
  }
  printf("\n");
}

void print_menu(RimeMenu* menu) {
  if (menu->num_candidates == 0)
    return;
  printf("page: %d%c (of size %d)\n", menu->page_no + 1,
         menu->is_last_page ? '$' : ' ', menu->page_size);
  for (int i = 0; i < menu->num_candidates; ++i) {
    bool highlighted = i == menu->highlighted_candidate_index;
    printf("%d. %c%s%c%s\n", i + 1, highlighted ? '[' : ' ',
           menu->candidates[i].text, highlighted ? ']' : ' ',
           menu->candidates[i].comment ? menu->candidates[i].comment : "");
  }
}

void print_context(RimeContext* context) {
  if (context->composition.length > 0 || context->menu.num_candidates > 0) {
    print_composition(&context->composition);
  } else {
    printf("(not composing)\n");
  }
  print_menu(&context->menu);
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
        printf("%zu. %s [%s]\n", (i + 1), list.list[i].name,
               list.list[i].schema_id);
      }
      rime->free_schema_list(&list);
    }
    char current[100] = {0};
    if (rime->get_current_schema(session_id, current, sizeof(current))) {
      printf("current schema: [%s]\n", current);
    }
    return true;
  }
  const char* kSelectSchemaCommand = "select schema ";
  size_t command_length = strlen(kSelectSchemaCommand);
  if (!strncmp(line, kSelectSchemaCommand, command_length)) {
    const char* schema_id = line + command_length;
    if (rime->select_schema(session_id, schema_id)) {
      printf("selected schema: [%s]\n", schema_id);
    }
    return true;
  }
  const char* kSelectCandidateCommand = "select candidate ";
  command_length = strlen(kSelectCandidateCommand);
  if (!strncmp(line, kSelectCandidateCommand, command_length)) {
    int index = atoi(line + command_length);
    if (index > 0 &&
        rime->select_candidate_on_current_page(session_id, index - 1)) {
      print(session_id);
    } else {
      fprintf(stderr, "cannot select candidate at index %d.\n", index);
    }
    return true;
  }
  if (!strcmp(line, "print candidate list")) {
    RimeCandidateListIterator iterator = {0};
    if (rime->candidate_list_begin(session_id, &iterator)) {
      while (rime->candidate_list_next(&iterator)) {
        printf("%d. %s", iterator.index + 1, iterator.candidate.text);
        if (iterator.candidate.comment)
          printf(" (%s)", iterator.candidate.comment);
        putchar('\n');
      }
      rime->candidate_list_end(&iterator);
    } else {
      printf("no candidates.\n");
    }
    return true;
  }
  const char* kSetOptionCommand = "set option ";
  command_length = strlen(kSetOptionCommand);
  if (!strncmp(line, kSetOptionCommand, command_length)) {
    Bool is_on = True;
    const char* option = line + command_length;
    if (*option == '!') {
      is_on = False;
      ++option;
    }
    rime->set_option(session_id, option, is_on);
    printf("%s set %s.\n", option, is_on ? "on" : "off");
    return true;
  }
  if (!strcmp(line, "synchronize")) {
    return rime->sync_user_data();
  }
  const char* kDeleteCandidateOnCurrentPage = "delete on current page ";
  command_length = strlen(kDeleteCandidateOnCurrentPage);
  if (!strncmp(line, kDeleteCandidateOnCurrentPage, command_length)) {
    const char* index_str = line + command_length;
    int index = atoi(index_str);
    if (!rime->delete_candidate_on_current_page(session_id, index)) {
      fprintf(stderr, "failed to delete\n");
    }
    return true;
  }
  const char* kDeleteCandidate = "delete ";
  command_length = strlen(kDeleteCandidate);
  if (!strncmp(line, kDeleteCandidate, command_length)) {
    const char* index_str = line + command_length;
    int index = atoi(index_str);
    if (!rime->delete_candidate(session_id, index)) {
      fprintf(stderr, "failed to delete\n");
    }
    return true;
  }
  return false;
}

void on_message(void* context_object,
                RimeSessionId session_id,
                const char* message_type,
                const char* message_value) {
  printf("message: [%zu] [%s] %s\n", session_id, message_type, message_value);
  RimeApi* rime = rime_get_api();
  if (RIME_API_AVAILABLE(rime, get_state_label) &&
      !strcmp(message_type, "option")) {
    Bool state = message_value[0] != '!';
    const char* option_name = message_value + !state;
    const char* state_label =
        rime->get_state_label(session_id, option_name, state);
    if (state_label) {
      printf("updated option: %s = %d // %s\n", option_name, state,
             state_label);
    }
  }
}

RimeSessionId ensure_session(RimeApi* rime) {
  RimeSessionId id = rime->create_session();
  if (!id) {
    fprintf(stderr, "Error creating rime session.\n");
  }
  return id;
}

int main(int argc, char* argv[]) {
  unsigned int codepage = SetConsoleOutputCodePage();
  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "rime.console";
  rime->setup(&traits);

  rime->set_notification_handler(&on_message, NULL);

  fprintf(stderr, "initializing...\n");
reload:
  rime->initialize(NULL);
  Bool full_check = True;
  if (rime->start_maintenance(full_check))
    rime->join_maintenance_thread();
  fprintf(stderr, "ready.\n");

  RimeSessionId session_id = 0;
  const int kMaxLength = 99;
  LineEditor editor(kMaxLength);
  std::string line;
  while (true) {
    if (!rime->find_session(session_id) &&
        !(session_id = ensure_session(rime))) {
      SetConsoleOutputCodePage(codepage);
      return 1;
    }
    editor.SetCompletions(BuildCompletions(rime, session_id));
    std::string header = MakeStatusHeader(rime, session_id);
    if (!editor.ReadLine("> ", &line, nullptr, nullptr, header.c_str()))
      break;
    if (line.empty())
      line = "\r";
    if (!strcmp(line.c_str(), "exit"))
      break;
    else if (!strcmp(line.c_str(), "reload")) {
      rime->destroy_session(session_id);
      rime->finalize();
      goto reload;
    }
    if (execute_special_command(line.c_str(), session_id))
      continue;
    if (rime->simulate_key_sequence(session_id, line.c_str())) {
      print(session_id);
    } else {
      fprintf(stderr, "Error processing key sequence: %s\n", line.c_str());
    }
  }

  rime->destroy_session(session_id);

  rime->finalize();

  SetConsoleOutputCodePage(codepage);
  return 0;
}
