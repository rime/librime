/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 */
#include "completion_catalog.h"

#include <string>
#include <vector>

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
    "Shift_L",      "Shift_R",    "Control_L",  "Control_R",  "Alt_L",
    "Alt_R",        "Super_L",    "Super_R",    "Meta_L",     "Meta_R",
    "Hyper_L",      "Hyper_R",    "grave",      "space",      "Tab",
    "Return",       "BackSpace",  "Escape",     "Left",       "Right",
    "Up",           "Down",       "Home",       "End",        "Page_Up",
    "Page_Down",    "Delete",     "Insert",     "minus",      "equal",
    "plus",         "underscore", "parenleft",  "parenright", "bracketleft",
    "bracketright", "braceleft",  "braceright", "less",       "greater",
    "colon",        "semicolon",  "comma",      "period",     "slash",
    "backslash",    "bar",        "question",   "exclam",     "quotedbl",
    "apostrophe",   "quoteleft",  "quoteright", "numbersign", "dollar",
    "percent",      "ampersand",  "asterisk",   "at",         "asciitilde",
    "asciicircum",
};

const char* const kModifierOrders[] = {
    "Shift",
    "Control",
    "Alt",
    "Super",
    "Release",
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

}  // namespace

CompletionCatalog::CompletionCatalog() {
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

void CompletionCatalog::AddCommand(const std::string& command) {
  Add(command, command, "console command", "command");
}

void CompletionCatalog::AddKey(const std::string& key) {
  Add("{" + key + "}", "{" + key + "}", "", "key");
}

void CompletionCatalog::Add(const std::string& text,
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
