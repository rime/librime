#include "line_editor.h"

#include <cctype>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <cstdio>

LineEditor::LineEditor(size_t max_length) : max_length_(max_length) {}

bool LineEditor::ReadLine(std::string* out) {
  if (!out)
    return false;
#ifndef _WIN32
  struct TermiosGuard {
    termios original;
    bool active = false;
    TermiosGuard() {
      if (!isatty(STDIN_FILENO))
        return;
      if (tcgetattr(STDIN_FILENO, &original) == 0) {
        termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
          active = true;
        }
      }
    }
    ~TermiosGuard() {
      if (active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
      }
    }
  } guard;
#endif
  std::string line;
  size_t cursor = 0;
  history_position_ = history_.size();
  browsing_history_ = false;
  saved_line_.clear();
  suggestion_.clear();
  last_rendered_length_ = 0;
  RefreshLine(line, cursor);
  while (true) {
    int ch = ReadChar();
    if (ch == -1) {
      putchar('\n');
      fflush(stdout);
      return false;
    }
    if (ch == '\r' || ch == '\n') {
      RefreshLine(line, line.size());
      putchar('\n');
      fflush(stdout);
      *out = line;
      if (!out->empty()) {
        bool whitespace_only = true;
        for (char ch : *out) {
          if (!std::isspace(static_cast<unsigned char>(ch))) {
            whitespace_only = false;
            break;
          }
        }
        if (!whitespace_only) {
          if (history_.empty() || history_.back() != *out) {
            history_.push_back(*out);
          }
        }
      }
      suggestion_.clear();
      last_rendered_length_ = 0;
      return true;
    }
    if ((ch == 4 || ch == 26) && line.empty()) {  // Ctrl-D / Ctrl-Z
      putchar('\n');
      fflush(stdout);
      return false;
    }
    if (ch == 3) {  // Ctrl-C
      putchar('\n');
      fflush(stdout);
      return false;
    }
    if (ch == 127 || ch == 8) {  // backspace
      if (cursor > 0) {
        line.erase(cursor - 1, 1);
        --cursor;
        UpdateSuggestion(line);
        RefreshLine(line, cursor);
      } else {
        EmitBell();
      }
      continue;
    }
    if (ch == 9) {  // Tab
      if (!suggestion_.empty()) {
        line.append(suggestion_);
        cursor = line.size();
        suggestion_.clear();
        RefreshLine(line, cursor);
      }
      continue;
    }
    if (HandleEscapeSequence(ch, &line, &cursor))
      continue;
    if (IsPrintable(ch)) {
      if (line.size() >= max_length_) {
        EmitBell();
        continue;
      }
      line.insert(cursor, 1, static_cast<char>(ch));
      ++cursor;
      UpdateSuggestion(line);
      RefreshLine(line, cursor);
      // editing after history recall should detach from history
      if (browsing_history_) {
        browsing_history_ = false;
        history_position_ = history_.size();
      }
      continue;
    }
    // ignore other keys
  }
}

int LineEditor::ReadChar() {
#ifdef _WIN32
  int ch = _getch();
  return ch;
#else
  unsigned char c = 0;
  ssize_t n = read(STDIN_FILENO, &c, 1);
  if (n <= 0)
    return -1;
  return static_cast<int>(c);
#endif
}

void LineEditor::RefreshLine(const std::string& line, size_t cursor) {
  // Carriage return to start of line
  putchar('\r');
  // Print current line content
  fputs(line.c_str(), stdout);

  // Print suggestion in dim color if available
  if (!suggestion_.empty() && cursor == line.length()) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD originalAttrs = consoleInfo.wAttributes;
    // Use Dark Gray (Bright Black) for suggestion
    // Preserve background
    WORD bg = originalAttrs & (BACKGROUND_BLUE | BACKGROUND_GREEN |
                               BACKGROUND_RED | BACKGROUND_INTENSITY);
    SetConsoleTextAttribute(hConsole, bg | FOREGROUND_INTENSITY);
    fputs(suggestion_.c_str(), stdout);
    SetConsoleTextAttribute(hConsole, originalAttrs);
#else
    // ANSI escape code for dim text (usually gray)
    fputs("\x1b[90m", stdout);
    fputs(suggestion_.c_str(), stdout);
    // Reset color
    fputs("\x1b[0m", stdout);
#endif
  }

  // Clear to end of line if previous line was longer
  size_t current_length = line.length() + suggestion_.length();
  if (current_length < last_rendered_length_) {
#ifdef _WIN32
    // Print spaces to clear
    for (size_t i = current_length; i < last_rendered_length_; ++i) {
      putchar(' ');
    }
    // Move cursor back to end of current content
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    COORD pos = consoleInfo.dwCursorPosition;
    pos.X -= static_cast<SHORT>(last_rendered_length_ - current_length);
    SetConsoleCursorPosition(hConsole, pos);
#else
    // ANSI escape code to clear from cursor to end of line
    fputs("\x1b[K", stdout);
#endif
  }
  last_rendered_length_ = current_length;

  // Move cursor to correct position
  // We are currently at the end of the printed content (line + suggestion)
  // We need to move back by (line.length() - cursor) + suggestion.length()
  size_t move_back = (line.length() - cursor) + suggestion_.length();
  if (move_back > 0) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    COORD pos = consoleInfo.dwCursorPosition;
    pos.X -= static_cast<SHORT>(move_back);
    SetConsoleCursorPosition(hConsole, pos);
#else
    printf("\x1b[%zuD", move_back);
#endif
  }

  fflush(stdout);
}
bool LineEditor::HandleEscapeSequence(int ch,
                                      std::string* line,
                                      size_t* cursor) {
  if (!line || !cursor)
    return false;
#ifdef _WIN32
  if (ch == 0 || ch == 224) {
    int next = _getch();
    switch (next) {
      case 72:  // up
        RecallHistory(-1, line, cursor);
        return true;
      case 80:  // down
        RecallHistory(1, line, cursor);
        return true;
      case 75:  // left
        if (*cursor > 0) {
          --(*cursor);
          RefreshLine(*line, *cursor);
        } else {
          EmitBell();
        }
        return true;
      case 77:  // right
        if (*cursor < line->size()) {
          ++(*cursor);
          RefreshLine(*line, *cursor);
        } else if (!suggestion_.empty()) {
          line->append(suggestion_);
          *cursor = line->size();
          suggestion_.clear();
          RefreshLine(*line, *cursor);
        } else {
          EmitBell();
        }
        return true;
      default:
        break;
    }
  }
#else
  if (ch == 27) {
    int next1 = ReadChar();
    if (next1 == -1)
      return true;
    if (next1 == '[') {
      int next2 = ReadChar();
      if (next2 == -1)
        return true;
      switch (next2) {
        case 'A':
          RecallHistory(-1, line, cursor);
          return true;
        case 'B':
          RecallHistory(1, line, cursor);
          return true;
        case 'C':
          if (*cursor < line->size()) {
            ++(*cursor);
            RefreshLine(*line, *cursor);
          } else if (!suggestion_.empty()) {
            line->append(suggestion_);
            *cursor = line->size();
            suggestion_.clear();
            RefreshLine(*line, *cursor);
          } else {
            EmitBell();
          }
          return true;
        case 'D':
          if (*cursor > 0) {
            --(*cursor);
            RefreshLine(*line, *cursor);
          } else {
            EmitBell();
          }
          return true;
        default:
          break;
      }
    }
    return true;
  }
#endif
  return false;
}

void LineEditor::RecallHistory(int direction,
                               std::string* line,
                               size_t* cursor) {
  if (history_.empty()) {
    EmitBell();
    return;
  }
  suggestion_.clear();
  if (!browsing_history_) {
    browsing_history_ = true;
    saved_line_ = *line;
    history_position_ = history_.size();
  }

  size_t pos = history_position_;
  if (direction < 0) {
    while (pos > 0) {
      --pos;
      if (history_[pos].compare(0, saved_line_.size(), saved_line_) == 0) {
        history_position_ = pos;
        *line = history_[history_position_];
        *cursor = line->size();
        RefreshLine(*line, *cursor);
        return;
      }
    }
  } else {
    while (pos < history_.size()) {
      ++pos;
      if (pos == history_.size()) {
        *line = saved_line_;
        browsing_history_ = false;
        history_position_ = history_.size();
        *cursor = line->size();
        RefreshLine(*line, *cursor);
        return;
      }
      if (history_[pos].compare(0, saved_line_.size(), saved_line_) == 0) {
        history_position_ = pos;
        *line = history_[history_position_];
        *cursor = line->size();
        RefreshLine(*line, *cursor);
        return;
      }
    }
  }
  EmitBell();
}

void LineEditor::UpdateSuggestion(const std::string& line) {
  suggestion_.clear();
  if (line.empty()) {
    return;
  }

  // Search history backwards for a match
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    if (it->length() > line.length() && it->substr(0, line.length()) == line) {
      suggestion_ = it->substr(line.length());
      return;
    }
  }
}

bool LineEditor::IsPrintable(int ch) const {
  if (ch < 0)
    return false;
  unsigned char uc = static_cast<unsigned char>(ch);
  return uc >= 32 && uc <= 126 && std::isprint(uc);
}

void LineEditor::EmitBell() const {
  putchar('\a');
  fflush(stdout);
}
