#pragma once

#include <cstddef>
#include <string>
#include <vector>

class LineEditor {
 public:
  explicit LineEditor(size_t max_length);

  // Reads one line into out; returns false on EOF or interruption.
  bool ReadLine(std::string* out);

 private:
  int ReadChar();
  bool HandleEscapeSequence(int ch, std::string* line, size_t* cursor);
  void RecallHistory(int direction, std::string* line, size_t* cursor);
  void RefreshLine(const std::string& line, size_t cursor);
  void UpdateSuggestion(const std::string& line);
  bool IsPrintable(int ch) const;
  void EmitBell() const;

  size_t max_length_ = 0;
  std::vector<std::string> history_;
  size_t history_position_ = 0;
  bool browsing_history_ = false;
  std::string saved_line_;
  std::string suggestion_;
  size_t last_rendered_length_ = 0;
};
