#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct LineCompletion {
  std::string text;
  std::string display;
  std::string detail;
  std::string kind;
};

class LineEditor {
 public:
  explicit LineEditor(size_t max_length);

  // Reads one line into out; returns false on EOF or interruption.
  // `header` is an optional status block rendered above the prompt.
  bool ReadLine(const char* prompt,
                std::string* out,
                const char* context = nullptr,
                const char* continuation_prompt = nullptr,
                const char* header = nullptr);
  void SetHistory(const std::vector<std::string>& history);
  void SetCompletions(const std::vector<LineCompletion>& completions);

 private:
  enum class SpecialKey {
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    Delete,
    PageUp,
    PageDown
  };

  struct CompletionContext {
    bool valid = false;
    bool whole_line = false;
    size_t begin = 0;
    size_t end = 0;
    std::string prefix;
  };

  int ReadChar();
  int ReadCharTimeout(int timeout_ms);
  bool HandleEscapeSequence(int ch, std::string* line, size_t* cursor);
  bool HandleAnsiSequence(int first, std::string* line, size_t* cursor);
  bool HandleSpecialKey(SpecialKey key, std::string* line, size_t* cursor);
  void RecallHistory(int direction, std::string* line, size_t* cursor);
  void RefreshLine(const std::string& line, size_t cursor);
  void UpdateSuggestion(const std::string& line, size_t cursor);

  bool ComputeCompletionContext(const std::string& line,
                                size_t cursor,
                                CompletionContext* ctx) const;
  bool RebuildMenu(const std::string& line, size_t cursor);
  void CloseMenu();
  void MoveMenu(int delta);
  void AcceptMenu(std::string* line, size_t* cursor);
  void AfterEdit(const std::string& line, size_t cursor);

  size_t TerminalWidth() const;
  size_t TerminalHeight() const;
  size_t MenuRows() const;
  size_t RenderMenu(size_t width);

  bool IsPrintable(int ch) const;
  void EmitBell() const;

  size_t max_length_ = 0;
  std::string prompt_;
  std::string continuation_prompt_;
  std::string context_;
  std::string header_;
  std::string header_visual_;
  bool fallback_header_printed_ = false;
  std::vector<std::string> history_;
  std::vector<LineCompletion> completions_;
  size_t history_position_ = 0;
  bool browsing_history_ = false;
  std::string saved_line_;
  std::string suggestion_;
  std::string last_line_render_;
  size_t last_cursor_row_ = 0;
  bool vt_supported_ = false;
  bool win_console_input_ = false;
  bool win_console_output_ = false;

  bool menu_open_ = false;
  std::vector<LineCompletion> menu_items_;
  size_t menu_selected_ = 0;
  size_t menu_scroll_ = 0;
  size_t menu_begin_ = 0;
  size_t menu_end_ = 0;
  std::string menu_prefix_;
};
