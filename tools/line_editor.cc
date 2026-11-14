#include "line_editor.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

namespace {

constexpr int kReadTimeout = -2;
constexpr int kReadInterrupted = -3;

#ifndef _WIN32
volatile sig_atomic_t g_window_changed = 0;

void HandleWindowChange(int) {
  g_window_changed = 1;
}
#endif

// ---------------------------------------------------------------------------
// UTF-8 and display width helpers.  The editor works on byte offsets, but all
// cursor movement and menu layout work on code-point/display-column boundaries.
// ---------------------------------------------------------------------------

size_t DecodeUtf8(const std::string& s, size_t pos, unsigned int* cp) {
  const unsigned char c = static_cast<unsigned char>(s[pos]);
  if (c < 0x80) {
    *cp = c;
    return 1;
  }
  size_t len = 0;
  unsigned int value = 0;
  if ((c & 0xE0) == 0xC0) {
    len = 2;
    value = c & 0x1F;
  } else if ((c & 0xF0) == 0xE0) {
    len = 3;
    value = c & 0x0F;
  } else if ((c & 0xF8) == 0xF0) {
    len = 4;
    value = c & 0x07;
  } else {
    *cp = 0xFFFD;
    return 1;
  }
  if (pos + len > s.size()) {
    *cp = 0xFFFD;
    return 1;
  }
  for (size_t i = 1; i < len; ++i) {
    const unsigned char cc = static_cast<unsigned char>(s[pos + i]);
    if ((cc & 0xC0) != 0x80) {
      *cp = 0xFFFD;
      return 1;
    }
    value = (value << 6) | (cc & 0x3F);
  }
  *cp = value;
  return len;
}

size_t PrevCodepoint(const std::string& s, size_t pos) {
  if (pos == 0)
    return 0;
  --pos;
  while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
    --pos;
  return pos;
}

size_t NextCodepoint(const std::string& s, size_t pos) {
  if (pos >= s.size())
    return s.size();
  unsigned int cp = 0;
  return std::min(s.size(), pos + DecodeUtf8(s, pos, &cp));
}

bool IsCombining(unsigned int cp) {
  return (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
         (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
         (cp >= 0xFE20 && cp <= 0xFE2F);
}

bool IsWide(unsigned int cp) {
  return (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E) ||
         (cp >= 0x3041 && cp <= 0x33FF) || (cp >= 0x3400 && cp <= 0x4DBF) ||
         (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xA000 && cp <= 0xA4CF) ||
         (cp >= 0xA960 && cp <= 0xA97F) || (cp >= 0xAC00 && cp <= 0xD7A3) ||
         (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE10 && cp <= 0xFE19) ||
         (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF60) ||
         (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x1F1E6 && cp <= 0x1F1FF) ||
         (cp >= 0x1F300 && cp <= 0x1FAFF) || (cp >= 0x20000 && cp <= 0x3FFFD);
}

size_t CodepointWidth(unsigned int cp) {
  if (cp == 0)
    return 0;
  if (cp < 32 || (cp >= 0x7F && cp < 0xA0))
    return 0;
  if (IsCombining(cp))
    return 0;
  if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF)
    return 0;
  if (IsWide(cp))
    return 2;
  return 1;
}

size_t DisplayWidth(const std::string& text) {
  size_t width = 0;
  size_t i = 0;
  while (i < text.size()) {
    unsigned int cp = 0;
    const size_t len = DecodeUtf8(text, i, &cp);
    width += CodepointWidth(cp);
    i += len;
  }
  return width;
}

std::string TruncateToWidth(const std::string& text, size_t max_width) {
  if (max_width == 0)
    return std::string();
  std::string out;
  size_t width = 0;
  size_t i = 0;
  while (i < text.size()) {
    unsigned int cp = 0;
    const size_t len = DecodeUtf8(text, i, &cp);
    const size_t w = CodepointWidth(cp);
    if (width + w > max_width)
      break;
    out.append(text, i, len);
    width += w;
    i += len;
  }
  return out;
}

struct Measure {
  size_t rows = 0;
  size_t col = 0;
};

// Measures a string assuming it starts at row 0, column start_col, and that
// the terminal wraps at `width` display columns.
Measure MeasureText(const std::string& text, size_t start_col, size_t width) {
  Measure m;
  m.col = start_col;
  size_t i = 0;
  while (i < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    if (c == '\n') {
      ++m.rows;
      m.col = 0;
      ++i;
      continue;
    }
    if (c == '\r') {
      ++i;
      continue;
    }
    unsigned int cp = 0;
    const size_t len = DecodeUtf8(text, i, &cp);
    const size_t w = CodepointWidth(cp);
    if (width > 0 && m.col > 0 && m.col + w > width) {
      ++m.rows;
      m.col = 0;
    }
    m.col += w;
    i += len;
  }
  return m;
}

std::string VisualTransform(const std::string& text,
                            const std::string& continuation_prompt) {
  std::string visual = text;
  const std::string replacement = "\n" + continuation_prompt;
  size_t pos = 0;
  while ((pos = visual.find('\n', pos)) != std::string::npos) {
    visual.replace(pos, 1, replacement);
    pos += replacement.length();
  }
  return visual;
}

std::string FirstWordSuffix(const std::string& suggestion) {
  size_t pos = 0;
  while (pos < suggestion.size() &&
         std::isspace(static_cast<unsigned char>(suggestion[pos])))
    ++pos;
  while (pos < suggestion.size() &&
         !std::isspace(static_cast<unsigned char>(suggestion[pos])))
    ++pos;
  if (pos < suggestion.size())
    ++pos;
  return suggestion.substr(0, pos);
}

std::string InferKind(const LineCompletion& completion) {
  if (!completion.kind.empty())
    return completion.kind;
  if (!completion.text.empty() && completion.text[0] == '{')
    return "key";
  return "command";
}

}  // namespace

LineEditor::LineEditor(size_t max_length) : max_length_(max_length) {
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
      win_console_output_ = true;
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      if (SetConsoleMode(hOut, dwMode))
        vt_supported_ = true;
    }
  }
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  if (hIn != INVALID_HANDLE_VALUE) {
    DWORD mode = 0;
    if (GetConsoleMode(hIn, &mode))
      win_console_input_ = true;
  }
  // mintty/ConEmu/Windows Terminal may not expose a classic console handle
  // for the output side but still understand ANSI escape sequences.
  if (!vt_supported_) {
    const char* term = std::getenv("TERM");
    if ((term && std::strcmp(term, "dumb") != 0) || std::getenv("WT_SESSION") ||
        std::getenv("MSYSTEM"))
      vt_supported_ = true;
  }
#else
  const char* term = std::getenv("TERM");
  vt_supported_ =
      isatty(STDOUT_FILENO) && (!term || std::strcmp(term, "dumb") != 0);
#endif
}

void LineEditor::SetHistory(const std::vector<std::string>& history) {
  history_ = history;
  history_position_ = history_.size();
}

void LineEditor::SetCompletions(
    const std::vector<LineCompletion>& completions) {
  completions_ = completions;
}

bool LineEditor::ReadLine(const char* prompt,
                          std::string* out,
                          const char* context,
                          const char* continuation_prompt,
                          const char* header) {
  if (!out)
    return false;
  prompt_ = prompt ? prompt : "";
  if (continuation_prompt) {
    continuation_prompt_ = continuation_prompt;
  } else {
    const size_t last_non_space = prompt_.find_last_not_of(" ");
    if (last_non_space == std::string::npos) {
      continuation_prompt_ = prompt_;
    } else {
      const std::string prefix = prompt_.substr(0, last_non_space + 1);
      const std::string suffix = prompt_.substr(last_non_space + 1);
      continuation_prompt_ = prefix + prefix + suffix;
    }
  }
  context_ = context ? context : "";
  header_ = header ? header : "";
  header_visual_ = header_;
  if (!header_visual_.empty() && header_visual_.back() != '\n')
    header_visual_ += '\n';
  fallback_header_printed_ = false;
#ifndef _WIN32
  struct TermiosGuard {
    termios original;
    bool active = false;
    struct sigaction old_winch;
    bool winch_installed = false;

    TermiosGuard() {
      g_window_changed = 0;
      struct sigaction action;
      memset(&action, 0, sizeof(action));
      action.sa_handler = HandleWindowChange;
      sigemptyset(&action.sa_mask);
      action.sa_flags = 0;
      if (sigaction(SIGWINCH, &action, &old_winch) == 0)
        winch_installed = true;

      if (!isatty(STDIN_FILENO))
        return;
      if (tcgetattr(STDIN_FILENO, &original) == 0) {
        termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
          active = true;
      }
    }

    ~TermiosGuard() {
      if (winch_installed)
        sigaction(SIGWINCH, &old_winch, nullptr);
      if (active)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
    }
  } guard;
#endif
  std::string line;
  size_t cursor = 0;
  history_position_ = history_.size();
  browsing_history_ = false;
  saved_line_.clear();
  suggestion_.clear();
  last_cursor_row_ = 0;
  last_line_render_.clear();
  CloseMenu();

  UpdateSuggestion(line, cursor);
  RefreshLine(line, cursor);

  while (true) {
    const int ch = ReadChar();
    if (ch == kReadInterrupted) {
      if (vt_supported_) {
        fputs("\r\x1b[J", stdout);
        last_cursor_row_ = 0;
      }
      RefreshLine(line, cursor);
      continue;
    }
    if (ch == -1) {
      putchar('\n');
      fflush(stdout);
      return false;
    }
    if (ch == '\r' || ch == '\n') {
      if (menu_open_) {
        AcceptMenu(&line, &cursor);
        UpdateSuggestion(line, cursor);
        RefreshLine(line, cursor);
        continue;
      }
      suggestion_.clear();
      RefreshLine(line, line.size());
      putchar('\n');
      fflush(stdout);
      *out = line;
      if (!out->empty()) {
        bool whitespace_only = true;
        for (char c : *out) {
          if (!std::isspace(static_cast<unsigned char>(c))) {
            whitespace_only = false;
            break;
          }
        }
        if (!whitespace_only) {
          const auto it = std::find(history_.begin(), history_.end(), *out);
          if (it == history_.end())
            history_.push_back(*out);
          if (history_.size() > 4096)
            history_.erase(history_.begin());
        }
      }
      suggestion_.clear();
      last_cursor_row_ = 0;
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
    if (ch == 14 || ch == 16) {  // Ctrl-N / Ctrl-P
      if (menu_open_) {
        MoveMenu(ch == 14 ? 1 : -1);
        RefreshLine(line, cursor);
      }
      continue;
    }
    if (ch == 127 || ch == 8) {  // backspace
      if (cursor > 0) {
        const size_t prev = PrevCodepoint(line, cursor);
        line.erase(prev, cursor - prev);
        cursor = prev;
        AfterEdit(line, cursor);
      } else {
        EmitBell();
      }
      continue;
    }
    if (ch == 9) {  // Tab
      if (menu_open_) {
        AcceptMenu(&line, &cursor);
        UpdateSuggestion(line, cursor);
        RefreshLine(line, cursor);
      } else if (vt_supported_ && RebuildMenu(line, cursor)) {
        if (menu_items_.size() == 1) {
          AcceptMenu(&line, &cursor);
          UpdateSuggestion(line, cursor);
        }
        RefreshLine(line, cursor);
      } else if (!suggestion_.empty()) {
        line.append(FirstWordSuffix(suggestion_));
        cursor = line.size();
        UpdateSuggestion(line, cursor);
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
      AfterEdit(line, cursor);
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
  if (win_console_input_) {
    const size_t width = TerminalWidth();
    const size_t height = TerminalHeight();
    while (!_kbhit()) {
      Sleep(50);
      if (TerminalWidth() != width || TerminalHeight() != height)
        return kReadInterrupted;
    }
    return _getch();
  }
  unsigned char c = 0;
  const int n = _read(_fileno(stdin), &c, 1);
  if (n <= 0)
    return -1;
  return static_cast<int>(c);
#else
  const size_t width = TerminalWidth();
  const size_t height = TerminalHeight();
  unsigned char c = 0;
  while (true) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    const int r = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (r == 0) {
      if (TerminalWidth() != width || TerminalHeight() != height)
        return kReadInterrupted;
      continue;
    }
    if (r < 0) {
      if (errno == EINTR) {
        if (g_window_changed) {
          g_window_changed = 0;
          return kReadInterrupted;
        }
        continue;
      }
      return -1;
    }
    const ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n > 0)
      return static_cast<int>(c);
    if (n < 0 && errno == EINTR) {
      if (g_window_changed) {
        g_window_changed = 0;
        return kReadInterrupted;
      }
      continue;
    }
    return -1;
  }
#endif
}

int LineEditor::ReadCharTimeout(int timeout_ms) {
  // Returns kReadTimeout on timeout and kReadInterrupted on window resize.
#ifdef _WIN32
  if (win_console_input_) {
    const size_t width = TerminalWidth();
    const size_t height = TerminalHeight();
    const DWORD start = GetTickCount();
    while (!_kbhit()) {
      if (TerminalWidth() != width || TerminalHeight() != height)
        return kReadInterrupted;
      if (static_cast<int>(GetTickCount() - start) >= timeout_ms)
        return kReadTimeout;
      Sleep(5);
    }
    return _getch();
  }
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  const DWORD start = GetTickCount();
  while (true) {
    if (h != INVALID_HANDLE_VALUE && GetFileType(h) == FILE_TYPE_PIPE) {
      DWORD avail = 0;
      if (PeekNamedPipe(h, nullptr, 0, nullptr, &avail, nullptr) && avail > 0)
        return ReadChar();
    } else {
      return ReadChar();
    }
    if (static_cast<int>(GetTickCount() - start) >= timeout_ms)
      return kReadTimeout;
    Sleep(5);
  }
#else
  while (true) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    const int r = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (r == 0)
      return kReadTimeout;
    if (r < 0) {
      if (errno == EINTR) {
        if (g_window_changed) {
          g_window_changed = 0;
          return kReadInterrupted;
        }
        continue;
      }
      return -1;
    }
    return ReadChar();
  }
#endif
}

size_t LineEditor::TerminalWidth() const {
#ifdef _WIN32
  if (win_console_output_) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
      const size_t width =
          static_cast<size_t>(info.srWindow.Right - info.srWindow.Left + 1);
      if (width > 0)
        return width;
    }
  }
#else
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    return ws.ws_col;
#endif
  const char* cols = std::getenv("COLUMNS");
  if (cols) {
    const int value = std::atoi(cols);
    if (value > 0)
      return static_cast<size_t>(value);
  }
  return 80;
}

size_t LineEditor::TerminalHeight() const {
#ifdef _WIN32
  if (win_console_output_) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
      const size_t height =
          static_cast<size_t>(info.srWindow.Bottom - info.srWindow.Top + 1);
      if (height > 0)
        return height;
    }
  }
#else
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0)
    return ws.ws_row;
#endif
  const char* lines = std::getenv("LINES");
  if (lines) {
    const int value = std::atoi(lines);
    if (value > 0)
      return static_cast<size_t>(value);
  }
  return 24;
}

size_t LineEditor::MenuRows() const {
  if (!menu_open_ || menu_items_.empty())
    return 0;
  const size_t height = TerminalHeight();
  size_t max_rows = height > 6 ? height / 3 : 3;
  max_rows = std::max<size_t>(3, std::min<size_t>(max_rows, 10));
  return std::min(menu_items_.size(), max_rows);
}

size_t LineEditor::RenderMenu(size_t width) {
  if (!menu_open_ || menu_items_.empty() || width == 0)
    return 0;
  const size_t n = menu_items_.size();
  const size_t visible = MenuRows();
  if (menu_selected_ < menu_scroll_)
    menu_scroll_ = menu_selected_;
  if (menu_selected_ >= menu_scroll_ + visible)
    menu_scroll_ = menu_selected_ - visible + 1;
  if (menu_scroll_ + visible > n)
    menu_scroll_ = n > visible ? n - visible : 0;

  for (size_t i = 0; i < visible; ++i) {
    const size_t index = menu_scroll_ + i;
    const LineCompletion& item = menu_items_[index];
    const std::string display = item.display.empty() ? item.text : item.display;
    const std::string kind = InferKind(item);
    const char* marker = "   ";
    if (kind == "key")
      marker = "{} ";
    else if (kind == "command" || kind == "cmd" || kind == "console")
      marker = ">  ";

    std::string line = "  ";
    line += marker;
    const size_t indent_width = DisplayWidth(line);
    const size_t max_display_width =
        width > indent_width ? width - indent_width : 0;
    const std::string truncated = TruncateToWidth(display, max_display_width);
    const size_t display_width = DisplayWidth(truncated);
    line += truncated;

    const bool selected = (index == menu_selected_);
    if (!item.detail.empty()) {
      const size_t detail_width = DisplayWidth(item.detail);
      const size_t used = indent_width + display_width;
      if (width > used + 1 && detail_width <= width - used - 1) {
        line += std::string(width - used - detail_width, ' ');
        if (selected)
          line += item.detail;
        else
          line += "\x1b[90m" + item.detail + "\x1b[0m";
      }
    }

    fputs("\r\n", stdout);
    if (selected) {
      fputs("\x1b[7m", stdout);
      fputs(line.c_str(), stdout);
      fputs("\x1b[0m", stdout);
    } else {
      fputs(line.c_str(), stdout);
    }
  }
  return visible;
}

void LineEditor::RefreshLine(const std::string& line, size_t cursor) {
  const std::string visual_line = VisualTransform(line, continuation_prompt_);
  const std::string visual_suggestion =
      VisualTransform(suggestion_, continuation_prompt_);
  const size_t width = TerminalWidth();

  if (vt_supported_) {
    if (last_cursor_row_ > 0)
      printf("\x1b[%zuA", last_cursor_row_);
    putchar('\r');
    fputs("\x1b[0m", stdout);
    fputs("\x1b[J", stdout);

    if (!header_visual_.empty())
      fputs(header_visual_.c_str(), stdout);

    const std::string content = prompt_ + visual_line;
    fputs(content.c_str(), stdout);
    if (!visual_suggestion.empty()) {
      fputs("\x1b[90m", stdout);
      fputs(visual_suggestion.c_str(), stdout);
      fputs("\x1b[0m", stdout);
    }

    const Measure end = MeasureText(content + visual_suggestion, 0, width);
    const size_t header_rows =
        header_visual_.empty() ? 0 : MeasureText(header_visual_, 0, width).rows;
    size_t menu_rows = 0;
    if (menu_open_ && !menu_items_.empty())
      menu_rows = RenderMenu(width);

    const std::string cursor_prefix =
        prompt_ + VisualTransform(line.substr(0, cursor), continuation_prompt_);
    Measure target = MeasureText(cursor_prefix, 0, width);
    if (width > 0 && target.col >= width)
      target.col = width - 1;

    const size_t end_row = header_rows + end.rows + menu_rows;
    const size_t target_row = header_rows + target.rows;
    if (end_row > target_row)
      printf("\x1b[%zuA", end_row - target_row);
    putchar('\r');
    if (target.col > 0)
      printf("\x1b[%zuC", target.col);
    fflush(stdout);

    last_cursor_row_ = target_row;
  } else {
    if (!fallback_header_printed_ && !header_visual_.empty()) {
      fputs(header_visual_.c_str(), stdout);
      fallback_header_printed_ = true;
    }
    putchar('\r');
    if (!last_line_render_.empty()) {
      const size_t clear = DisplayWidth(last_line_render_) + 1;
      for (size_t i = 0; i < clear; ++i)
        putchar(' ');
      putchar('\r');
    }
    fputs(prompt_.c_str(), stdout);
    fputs(visual_line.c_str(), stdout);
    if (!visual_suggestion.empty()) {
#ifdef _WIN32
      if (win_console_output_) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
        const WORD originalAttrs = consoleInfo.wAttributes;
        const WORD bg = originalAttrs & (BACKGROUND_BLUE | BACKGROUND_GREEN |
                                         BACKGROUND_RED | BACKGROUND_INTENSITY);
        SetConsoleTextAttribute(hConsole, bg | FOREGROUND_INTENSITY);
        fputs(visual_suggestion.c_str(), stdout);
        SetConsoleTextAttribute(hConsole, originalAttrs);
      } else {
        fputs(visual_suggestion.c_str(), stdout);
      }
#else
      fputs(visual_suggestion.c_str(), stdout);
#endif
    }
    last_line_render_ = prompt_ + visual_line + visual_suggestion;
    fflush(stdout);
  }
}

bool LineEditor::HandleSpecialKey(SpecialKey key,
                                  std::string* line,
                                  size_t* cursor) {
  switch (key) {
    case SpecialKey::Up:
      if (menu_open_) {
        MoveMenu(-1);
        RefreshLine(*line, *cursor);
      } else {
        RecallHistory(-1, line, cursor);
      }
      return true;
    case SpecialKey::Down:
      if (menu_open_) {
        MoveMenu(1);
        RefreshLine(*line, *cursor);
      } else {
        RecallHistory(1, line, cursor);
      }
      return true;
    case SpecialKey::Left:
      if (menu_open_) {
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
      }
      if (*cursor > 0) {
        *cursor = PrevCodepoint(*line, *cursor);
        RefreshLine(*line, *cursor);
      } else {
        EmitBell();
      }
      return true;
    case SpecialKey::Right:
      if (menu_open_) {
        if (*cursor == line->size()) {
          AcceptMenu(line, cursor);
          UpdateSuggestion(*line, *cursor);
          RefreshLine(*line, *cursor);
          return true;
        }
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
      }
      if (*cursor < line->size()) {
        *cursor = NextCodepoint(*line, *cursor);
        RefreshLine(*line, *cursor);
      } else if (!suggestion_.empty()) {
        line->append(suggestion_);
        *cursor = line->size();
        UpdateSuggestion(*line, *cursor);
        RefreshLine(*line, *cursor);
      } else {
        EmitBell();
      }
      return true;
    case SpecialKey::Home:
      if (menu_open_) {
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
      }
      *cursor = 0;
      RefreshLine(*line, *cursor);
      return true;
    case SpecialKey::End:
      if (menu_open_) {
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
      }
      *cursor = line->size();
      UpdateSuggestion(*line, *cursor);
      RefreshLine(*line, *cursor);
      return true;
    case SpecialKey::Delete:
      if (menu_open_)
        CloseMenu();
      if (*cursor < line->size()) {
        const size_t next = NextCodepoint(*line, *cursor);
        line->erase(*cursor, next - *cursor);
        UpdateSuggestion(*line, *cursor);
        RefreshLine(*line, *cursor);
      } else {
        EmitBell();
      }
      return true;
    case SpecialKey::PageUp:
      if (menu_open_) {
        MoveMenu(-static_cast<int>(MenuRows()));
        RefreshLine(*line, *cursor);
      } else {
        RecallHistory(-1, line, cursor);
      }
      return true;
    case SpecialKey::PageDown:
      if (menu_open_) {
        MoveMenu(static_cast<int>(MenuRows()));
        RefreshLine(*line, *cursor);
      } else {
        RecallHistory(1, line, cursor);
      }
      return true;
  }
  return false;
}

bool LineEditor::HandleAnsiSequence(int first,
                                    std::string* line,
                                    size_t* cursor) {
  if (first != '[' && first != 'O')
    return false;
  const int next = ReadCharTimeout(60);
  if (next == kReadInterrupted) {
    RefreshLine(*line, *cursor);
    return true;
  }
  if (next == kReadTimeout || next == -1)
    return true;
  switch (next) {
    case 'A':
      return HandleSpecialKey(SpecialKey::Up, line, cursor);
    case 'B':
      return HandleSpecialKey(SpecialKey::Down, line, cursor);
    case 'C':
      return HandleSpecialKey(SpecialKey::Right, line, cursor);
    case 'D':
      return HandleSpecialKey(SpecialKey::Left, line, cursor);
    case 'H':
      return HandleSpecialKey(SpecialKey::Home, line, cursor);
    case 'F':
      return HandleSpecialKey(SpecialKey::End, line, cursor);
    default:
      break;
  }
  if (next >= '0' && next <= '9') {
    int code = next - '0';
    while (true) {
      const int c = ReadCharTimeout(60);
      if (c == kReadInterrupted) {
        RefreshLine(*line, *cursor);
        return true;
      }
      if (c == kReadTimeout || c == -1)
        return true;
      if (c == '~') {
        switch (code) {
          case 1:
          case 7:
            return HandleSpecialKey(SpecialKey::Home, line, cursor);
          case 3:
            return HandleSpecialKey(SpecialKey::Delete, line, cursor);
          case 4:
          case 8:
            return HandleSpecialKey(SpecialKey::End, line, cursor);
          case 5:
            return HandleSpecialKey(SpecialKey::PageUp, line, cursor);
          case 6:
            return HandleSpecialKey(SpecialKey::PageDown, line, cursor);
          default:
            return true;
        }
      }
      if (c >= '0' && c <= '9')
        code = code * 10 + (c - '0');
      else
        break;
    }
  }
  return true;
}

bool LineEditor::HandleEscapeSequence(int ch,
                                      std::string* line,
                                      size_t* cursor) {
  if (!line || !cursor)
    return false;
#ifdef _WIN32
  if (ch == 0 || ch == 224) {
    const int next = _getch();
    switch (next) {
      case 72:
        return HandleSpecialKey(SpecialKey::Up, line, cursor);
      case 80:
        return HandleSpecialKey(SpecialKey::Down, line, cursor);
      case 75:
        return HandleSpecialKey(SpecialKey::Left, line, cursor);
      case 77:
        return HandleSpecialKey(SpecialKey::Right, line, cursor);
      case 71:
        return HandleSpecialKey(SpecialKey::Home, line, cursor);
      case 79:
        return HandleSpecialKey(SpecialKey::End, line, cursor);
      case 83:
        return HandleSpecialKey(SpecialKey::Delete, line, cursor);
      case 73:
        return HandleSpecialKey(SpecialKey::PageUp, line, cursor);
      case 81:
        return HandleSpecialKey(SpecialKey::PageDown, line, cursor);
      default:
        return true;
    }
  }
  if (ch == 27) {
    const int next = ReadCharTimeout(60);
    if (next == kReadTimeout) {  // lone Escape
      if (menu_open_) {
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
        RefreshLine(*line, *cursor);
      }
      return true;
    }
    if (next == -1)
      return true;
    if (next == kReadInterrupted) {
      RefreshLine(*line, *cursor);
      return true;
    }
    return HandleAnsiSequence(next, line, cursor);
  }
#else
  if (ch == 27) {
    const int next = ReadCharTimeout(60);
    if (next == kReadTimeout) {  // lone Escape
      if (menu_open_) {
        CloseMenu();
        UpdateSuggestion(*line, *cursor);
        RefreshLine(*line, *cursor);
      }
      return true;
    }
    if (next == -1)
      return true;
    if (next == kReadInterrupted) {
      RefreshLine(*line, *cursor);
      return true;
    }
    return HandleAnsiSequence(next, line, cursor);
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

  const std::string search_prefix = context_ + saved_line_;

  size_t pos = history_position_;
  if (direction < 0) {
    while (pos > 0) {
      --pos;
      if (history_[pos].size() >= search_prefix.size() &&
          history_[pos].compare(0, search_prefix.size(), search_prefix) == 0) {
        history_position_ = pos;
        *line = history_[history_position_].substr(context_.length());
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
      if (history_[pos].size() >= search_prefix.size() &&
          history_[pos].compare(0, search_prefix.size(), search_prefix) == 0) {
        history_position_ = pos;
        *line = history_[history_position_].substr(context_.length());
        *cursor = line->size();
        RefreshLine(*line, *cursor);
        return;
      }
    }
  }
  EmitBell();
}

bool LineEditor::ComputeCompletionContext(const std::string& line,
                                          size_t cursor,
                                          CompletionContext* ctx) const {
  if (!ctx)
    return false;
  ctx->valid = false;
  if (cursor != line.size() || line.empty())
    return false;

  // Prefer a whole-line match so multi-word commands such as
  // "select schema " and "print candidate list" keep working.
  for (const auto& completion : completions_) {
    if (completion.text.size() > line.size() &&
        completion.text.compare(0, line.size(), line) == 0) {
      ctx->valid = true;
      ctx->whole_line = true;
      ctx->begin = 0;
      ctx->end = line.size();
      ctx->prefix = line;
      return true;
    }
  }

  size_t token_start = 0;
  if (cursor > 0) {
    const size_t space = line.find_last_of(" \t", cursor - 1);
    if (space != std::string::npos)
      token_start = space + 1;
  }
  if (token_start >= cursor)
    return false;
  const std::string token = line.substr(token_start, cursor - token_start);
  for (const auto& completion : completions_) {
    if (completion.text.size() > token.size() &&
        completion.text.compare(0, token.size(), token) == 0) {
      ctx->valid = true;
      ctx->whole_line = false;
      ctx->begin = token_start;
      ctx->end = cursor;
      ctx->prefix = token;
      return true;
    }
  }
  return false;
}

bool LineEditor::RebuildMenu(const std::string& line, size_t cursor) {
  CompletionContext ctx;
  if (!ComputeCompletionContext(line, cursor, &ctx)) {
    CloseMenu();
    return false;
  }

  std::vector<LineCompletion> items;
  items.reserve(32);
  for (const auto& completion : completions_) {
    if (completion.text.size() > ctx.prefix.size() &&
        completion.text.compare(0, ctx.prefix.size(), ctx.prefix) == 0) {
      items.push_back(completion);
      if (items.size() >= 256)
        break;
    }
  }
  if (items.empty()) {
    CloseMenu();
    return false;
  }

  if (!menu_open_ || menu_begin_ != ctx.begin || menu_prefix_ != ctx.prefix) {
    menu_selected_ = 0;
    menu_scroll_ = 0;
  }
  menu_items_ = std::move(items);
  menu_begin_ = ctx.begin;
  menu_end_ = ctx.end;
  menu_prefix_ = ctx.prefix;
  menu_open_ = true;
  if (menu_selected_ >= menu_items_.size())
    menu_selected_ = 0;

  const std::string& selected = menu_items_[menu_selected_].text;
  if (selected.size() > menu_prefix_.size())
    suggestion_ = selected.substr(menu_prefix_.size());
  else
    suggestion_.clear();
  return true;
}

void LineEditor::CloseMenu() {
  menu_open_ = false;
  menu_items_.clear();
  menu_selected_ = 0;
  menu_scroll_ = 0;
  menu_begin_ = 0;
  menu_end_ = 0;
  menu_prefix_.clear();
}

void LineEditor::MoveMenu(int delta) {
  if (!menu_open_ || menu_items_.empty())
    return;
  const int count = static_cast<int>(menu_items_.size());
  int selected = static_cast<int>(menu_selected_) + delta;
  selected %= count;
  if (selected < 0)
    selected += count;
  menu_selected_ = static_cast<size_t>(selected);
  const std::string& text = menu_items_[menu_selected_].text;
  if (text.size() > menu_prefix_.size())
    suggestion_ = text.substr(menu_prefix_.size());
  else
    suggestion_.clear();
}

void LineEditor::AcceptMenu(std::string* line, size_t* cursor) {
  if (!menu_open_ || menu_items_.empty())
    return;
  const std::string insert = menu_items_[menu_selected_].text;
  if (menu_begin_ <= line->size()) {
    const size_t count = menu_end_ > menu_begin_
                             ? std::min(menu_end_, line->size()) - menu_begin_
                             : std::string::npos;
    line->replace(menu_begin_, count, insert);
  }
  *cursor = line->size();
  CloseMenu();
  suggestion_.clear();
}

void LineEditor::AfterEdit(const std::string& line, size_t cursor) {
  if (menu_open_) {
    if (!RebuildMenu(line, cursor))
      UpdateSuggestion(line, cursor);
  } else {
    UpdateSuggestion(line, cursor);
  }
  RefreshLine(line, cursor);
}

void LineEditor::UpdateSuggestion(const std::string& line, size_t cursor) {
  suggestion_.clear();
  if (cursor != line.size())
    return;

  // Prefer explicit completions over history suggestions. First try a
  // whole-line prefix match, which supports multi-word commands such as
  // "select schema " and "print candidate list".
  if (!completions_.empty() && !line.empty()) {
    for (const auto& completion : completions_) {
      if (completion.text.length() > line.length() &&
          completion.text.compare(0, line.length(), line) == 0) {
        suggestion_ = completion.text.substr(line.length());
        return;
      }
    }
  }

  // Then complete the current whitespace-delimited token, so key sequences
  // such as "{Esc" work.
  if (!completions_.empty()) {
    size_t token_start = 0;
    if (cursor > 0) {
      const size_t space = line.find_last_of(" \t", cursor - 1);
      if (space != std::string::npos)
        token_start = space + 1;
    }
    if (token_start < cursor) {
      const std::string token = line.substr(token_start, cursor - token_start);
      for (const auto& completion : completions_) {
        if (completion.text.length() > token.length() &&
            completion.text.compare(0, token.length(), token) == 0) {
          suggestion_ = completion.text.substr(token.length());
          return;
        }
      }
    }
  }

  // Allow suggestion even if line is empty, as long as we have context
  // (previous lines).
  if (line.empty() && context_.empty())
    return;

  const std::string full_current = context_ + line;
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    if (it->length() > full_current.length() &&
        it->substr(0, full_current.length()) == full_current) {
      suggestion_ = it->substr(full_current.length());
      return;
    }
  }
}

bool LineEditor::IsPrintable(int ch) const {
  if (ch < 0)
    return false;
  const unsigned char uc = static_cast<unsigned char>(ch);
  return uc >= 32 && uc != 127;
}

void LineEditor::EmitBell() const {
  putchar('\a');
  fflush(stdout);
}
