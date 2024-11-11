#ifndef RIME_STRINGS_H_
#define RIME_STRINGS_H_

#include <rime/common.h>
#include <initializer_list>

namespace rime {
namespace strings {

namespace details {

template <typename T>
struct PieceTraits {
  static const T& forward(const T& t) { return t; }
};

template <typename T, size_t TN>
struct PieceTraits<T[TN]> {
  static pair<const char*, std::size_t> forward(const T (&t)[TN]) {
    static_assert(TN > 0, "No char array but only literal");
    return {t, TN - 1};
  }
};

template <typename T>
struct PieceTraits<T*> {
  static pair<const char*, std::size_t> forward(T* t) {
    return {t, std::char_traits<char>::length(t)};
  }
};

template <>
struct PieceTraits<string_view> {
  static pair<const char*, std::size_t> forward(string_view t) {
    return {t.data(), t.size()};
  }
};

class Piece {
 public:
  Piece(pair<const char*, std::size_t> p) : piece_(p.first), size_(p.second) {}

  template <typename T,
            typename = std::enable_if_t<std::is_same<T, char>::value>>
  Piece(const T* p) : piece_(p), size_(std::char_traits<T>::length(p)) {}

  Piece(const string& str) : piece_(str.data()), size_(str.size()) {}

  Piece(char c) = delete;
#define PIECE_NUMERIC_CONV(TYPE, FORMAT_STRING)                            \
  Piece(TYPE i) : piece_(buffer_) {                                        \
    auto size = snprintf(buffer_, sizeof(buffer_), FORMAT_STRING, i);      \
    assert(size >= 0 && static_cast<size_t>(size) + 1 <= sizeof(buffer_)); \
    size_ = size;                                                          \
  }
  PIECE_NUMERIC_CONV(int, "%d");
  PIECE_NUMERIC_CONV(unsigned int, "%u");
  PIECE_NUMERIC_CONV(long, "%ld");
  PIECE_NUMERIC_CONV(unsigned long, "%lu");
  PIECE_NUMERIC_CONV(long long, "%lld");
  PIECE_NUMERIC_CONV(unsigned long long, "%llu");
  PIECE_NUMERIC_CONV(float, "%f");
  PIECE_NUMERIC_CONV(double, "%lf");

  Piece(const Piece&) = delete;

  const char* piece() const { return piece_; }
  std::size_t size() const { return size_; }

  pair<const char*, std::size_t> ToPair() const { return {piece_, size_}; }

  pair<const char*, std::size_t> ToPathPair(
      const bool removePrefixSlash = true) const {
    const auto* piece = piece_;
    auto size = size_;
    // Consume prefix and suffix slash.
    if (removePrefixSlash) {
      while (size && piece[0] == '/') {
        ++piece;
        --size;
      }
    }
    while (size && piece[size - 1] == '/') {
      --size;
    }
    // If first component is all slash, keep all of them.
    if (size_ && !removePrefixSlash && !size) {
      return {piece_, size_};
    }

    assert(size > 0);
    return {piece, size};
  }

 private:
  static constexpr int IntegerBufferSize = 30;
  const char* piece_;
  std::size_t size_;
  char buffer_[IntegerBufferSize];
};

string ConcatPieces(std::initializer_list<pair<const char*, std::size_t>> list);
}  // namespace details

template <typename... Args>
string concat(const Args&... args) {
  using namespace details;
  return ConcatPieces(
      {static_cast<const Piece&>(PieceTraits<Args>::forward(args))
           .ToPair()...});
}

bool starts_with(string_view input, string_view prefix);

bool ends_with(string_view input, string_view suffix);

enum class SplitBehavior { KeepToken, SkipToken };

vector<string> split(const string& str,
                     const string& delim,
                     SplitBehavior behavior);

vector<string> split(const string& str, const string& delim);

template <typename Iter, typename T>
string join(Iter start, Iter end, T&& delim) {
  string result;
  if (start != end) {
    result += (*start);
    start++;
  }
  for (; start != end; start++) {
    result += (delim);
    result += (*start);
  }
  return result;
}

template <typename C, typename T>
inline string join(C&& container, T&& delim) {
  return join(std::begin(container), std::end(container), delim);
}

template <typename C, typename T>
inline string join(std::initializer_list<C>&& container, T&& delim) {
  return join(std::begin(container), std::end(container), delim);
}

}  // namespace strings
}  // namespace rime

#endif  // RIME_STRINGS_H_
