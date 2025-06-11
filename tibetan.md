# Tibetan Language Support

## Features

- Wylie transliteration input method
- Tibetan punctuation support
- Common Tibetan words dictionary

## Usage

1. Select "Tibetan (Wylie)" schema
2. Type Wylie transliteration:
   - `ka` → ཀ
   - `kha` → ཁ
   - `bod` → བོད་

## Development

To extend Tibetan support:

```cpp
// Register Tibetan components
RIME_REGISTER_COMPONENT(TibetanOrder);
RIME_REGISTER_COMPONENT(TibetanFilter);
```
