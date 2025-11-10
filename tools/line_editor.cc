#include "line_editor.h"

#include <cctype>
#ifdef _WIN32
#include <conio.h>
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
        RefreshLine(line, cursor);
      } else {
        EmitBell();
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
  if (!browsing_history_) {
    browsing_history_ = true;
    saved_line_ = *line;
    history_position_ = history_.size();
  }
  if (direction < 0) {
    if (history_position_ == 0) {
      EmitBell();
      return;
    }
    --history_position_;
    *line = history_[history_position_];
  } else {
    if (history_position_ + 1 >= history_.size()) {
      *line = saved_line_;
      browsing_history_ = false;
      history_position_ = history_.size();
    } else {
      ++history_position_;
      *line = history_[history_position_];
    }
  }
  *cursor = line->size();
  RefreshLine(*line, *cursor);
}

void LineEditor::RefreshLine(const std::string& line, size_t cursor) {
  printf("\r");
  fwrite(line.data(), 1, line.size(), stdout);
  size_t clear_width = 0;
  if (last_rendered_length_ > line.size()) {
    clear_width = last_rendered_length_ - line.size();
  }
  for (size_t i = 0; i < clear_width; ++i) {
    putchar(' ');
  }
  printf("\r");
  if (cursor > 0) {
    fwrite(line.data(), 1, cursor, stdout);
  }
  fflush(stdout);
  last_rendered_length_ = line.size();
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
