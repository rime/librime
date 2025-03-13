# Rime输入法转换规则详解

本文档详细介绍Rime输入法中的六种主要转换规则：`xlit`、`xform`、`derive`、`erase`、`fuzz`和`abbrev`。这些规则在Rime配置文件（如*.schema.yaml）中被广泛使用，用于定制输入法的行为和处理各种输入情况。

## 目录

- [规则概览](#规则概览)
- [详细解释](#详细解释)
  - [xlit（音译/字符替换）](#xlit音译字符替换)
  - [xform（变形/模式替换）](#xform变形模式替换)
  - [derive（派生/衍生）](#derive派生衍生)
  - [erase（擦除）](#erase擦除)
  - [fuzz（模糊处理）](#fuzz模糊处理)
  - [abbrev（缩写）](#abbrev缩写)
- [规则组合与使用场景](#规则组合与使用场景)
- [实例分析](#实例分析)
- [源码解析](#源码解析)

## 规则概览

| 规则名 | 功能描述 | 格式示例 | 应用场景 |
|-------|---------|----------|----------|
| xlit | 一对一字符替换 | `xlit/abcd/ABCD/` | 处理字符层面的变换，如去除拼音声调 |
| xform | 正则表达式替换 | `xform/pattern/replacement/` | 处理模式层面的变换，如拼音组合变换 |
| derive | 保留原始输入的变体生成 | `derive/pattern/replacement/` | 创建输入的替代形式，如拼音变体 |
| erase | 删除匹配特定模式的输入 | `erase/pattern/` | 清除无效或特殊的输入序列 |
| fuzz | 模糊匹配，降低排序 | `fuzz/pattern/replacement/` | 处理常见拼写错误或方言差异 |
| abbrev | 生成缩写形式，降低排序 | `abbrev/pattern/replacement/` | 处理简拼或首字母输入 |

## 详细解释

### xlit（音译/字符替换）

`xlit`是"transliteration"（音译/字符替换）的缩写，用于进行一对一的字符映射替换。

#### 格式

```
xlit/源字符序列/目标字符序列/
```

#### 工作原理

`xlit`将第一组中的每个字符替换为第二组中对应位置的字符。两组字符必须一一对应且长度相等。

#### 示例

```yaml
xlit/āáǎàōóǒòēéěèīíǐìūúǔùǖǘǚǜü/aaaaooooeeeeiiiiuuuuvvvvv/
```

这条规则将带声调的拼音字母替换为不带声调的普通字母。例如：
- `ā` → `a`
- `ǐ` → `i`
- `ǚ` → `v`

#### 源码实现

`xlit`在librime源码中由`Transliteration`类实现：

```cpp
class Transliteration : public Calculation {
 protected:
  map<uint32_t, uint32_t> char_map_;  // 字符映射表
};
```

它通过一个字符映射表实现一对一的字符替换。

### xform（变形/模式替换）

`xform`是"transform"（变形/转换）的缩写，用于基于正则表达式的模式匹配和替换。

#### 格式

```
xform/模式/替换/
```

#### 工作原理

`xform`使用正则表达式查找符合指定模式的字符串，并将其替换为新的字符串。

#### 示例

```yaml
xform/ń|ň|ǹ/en/
xform/\bng\b/eng/
```

第一条规则将`ń`、`ň`或`ǹ`替换为`en`。
第二条规则将独立的`ng`（前后都是单词边界`\b`）替换为`eng`。

#### 源码实现

`xform`在librime源码中由`Transformation`类实现：

```cpp
class Transformation : public Calculation {
 protected:
  boost::regex pattern_;  // 正则表达式模式
  string replacement_;    // 替换字符串
};
```

它通过正则表达式（`boost::regex_replace`）实现模式匹配和替换。

### derive（派生/衍生）

`derive`用于生成新的输入码变体，保留原始输入的同时，添加一种可选的输入方式。

#### 格式

```
derive/模式/衍生形式/
```

#### 工作原理

`derive`匹配指定模式的输入，然后生成一个新的变体，但不会替换原始输入。这样，用户可以使用多种方式输入同一个内容。

#### 示例

```yaml
derive/^([jqxy])u$/$1v/
```

这条规则为以`j`、`q`、`x`或`y`开头，以`u`结尾的编码（如`ju`、`qu`、`xu`、`yu`）派生出以`v`结尾的变体（变成`jv`、`qv`、`xv`、`yv`）。`$1`表示第一个括号捕获的内容。

#### 源码实现

`derive`在librime源码中由`Derivation`类实现，它继承自`Transformation`：

```cpp
class Derivation : public Transformation {
 public:
  bool deletion() { return false; }  // 重写deletion方法返回false
};
```

关键在于重写了`deletion()`方法返回`false`，表示它不会删除原始输入。

### erase（擦除）

`erase`用于完全删除符合特定模式的输入。

#### 格式

```
erase/模式/
```

#### 工作原理

当输入的编码完全匹配指定的正则表达式模式时，该编码会被清空（删除）。

#### 示例

```yaml
erase/^xx$/
```

这会删除只包含两个字母x的输入（如"xx"），但不会影响其他包含x的输入（如"xxa"）。

#### 源码实现

`erase`在librime源码中由`Erasion`类实现：

```cpp
class Erasion : public Calculation {
 public:
  bool addition() { return false; }  // 重写addition方法返回false
 protected:
  boost::regex pattern_;  // 正则表达式模式
};

bool Erasion::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;
  if (!boost::regex_match(spelling->str, pattern_))
    return false;
  spelling->str.clear();  // 清空字符串
  return true;
}
```

它重写了`addition()`方法返回`false`，表示它不会添加新的输入，而是删除现有输入。

### fuzz（模糊处理）

`fuzz`用于处理模糊匹配，允许用户输入不太准确的拼写。

#### 格式

```
fuzz/模式/替换/
```

#### 工作原理

`fuzz`与`derive`类似，但会将新生成的拼写标记为"模糊拼写"，并降低其可信度。这意味着模糊匹配的候选词在排序中会被降级。

#### 示例

```yaml
fuzz/^([zcs])h/$1/
```

这条规则允许用户省略声母zh、ch、sh中的h，输入z、c、s时也能匹配到zh、ch、sh开头的字词，但这些匹配的候选项会排在正确拼写的候选项之后。

#### 源码实现

`fuzz`在librime源码中由`Fuzzing`类实现，它继承自`Derivation`：

```cpp
class Fuzzing : public Derivation {
 public:
  bool Apply(Spelling* spelling);
};

bool Fuzzing::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.type = kFuzzySpelling;
    spelling->properties.credibility += kFuzzySpellingPenalty;  // 降低可信度
  }
  return result;
}

// 惩罚值定义
const double kFuzzySpellingPenalty = -0.6931471805599453;  // log(0.5)
```

它将匹配的拼写标记为模糊拼写，并添加惩罚值降低其排序。

### abbrev（缩写）

`abbrev`用于处理拼音的简写形式，如简拼。

#### 格式

```
abbrev/模式/缩写形式/
```

#### 工作原理

`abbrev`与`fuzz`类似，也会降低生成的缩写形式的排序权重，但它主要用于处理简拼或其他缩写形式。

#### 示例

```yaml
abbrev/^([a-z]).+$/$1/
```

这条规则允许用户只输入拼音的首字母来匹配完整拼音的字词。比如输入"z"可以匹配到"zhong"、"zai"等所有以z开头的拼音。

#### 源码实现

`abbrev`在librime源码中由`Abbreviation`类实现，它继承自`Derivation`：

```cpp
class Abbreviation : public Derivation {
 public:
  bool Apply(Spelling* spelling);
};

bool Abbreviation::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.type = kAbbreviation;
    spelling->properties.credibility += kAbbreviationPenalty;  // 降低可信度
  }
  return result;
}

// 惩罚值定义
const double kAbbreviationPenalty = -0.6931471805599453;  // log(0.5)
```

它将匹配的拼写标记为缩写，并添加惩罚值降低其排序。

## 规则组合与使用场景

这些规则通常不是孤立使用的，而是组合起来实现复杂的输入处理逻辑。以下是一些常见的组合使用场景：

### 拼音处理

```yaml
# 去除声调
- xlit/āáǎàōóǒòēéěèīíǐìūúǔùǖǘǚǜü/aaaaooooeeeeiiiiuuuuvvvvv/

# 处理特殊音节
- xform/ń|ň|ǹ/en/
- xform/\bng\b/eng/

# 允许u/v互换
- derive/^([jqxy])u$/$1v/

# 允许省略声调
- fuzz/ü/v/

# 支持简拼
- abbrev/^([a-z]).+$/$1/
```

### 鼠须管/朙月拼音的常见规则

```yaml
# 全拼与双拼共存
- derive/^([jqxy])u$/$1v/
- derive/^([aoe])(.*)$/o$1$2/

# 智能纠错
- fuzz/iang/iag/
- fuzz/^([zcs])h/$1/

# 支持简拼
- abbrev/^([a-z]).+$/$1/

# 特殊处理
- xform/erhua/er/
```

## 实例分析

以万象拼音中的一段配置为例：

```yaml
搜狗: 
  __append:
    - xlit/āáǎàōóǒòēéěèīíǐìūúǔùǖǘǚǜü/aaaaooooeeeeiiiiuuuuvvvvv/
    - xform/ń|ň|ǹ/en/ 
    - xform/\bng\b/eng/
    - xform/ńg|ňg|ǹg/eng/
    - derive/^([jqxy])u$/$1v/
    - derive/^([aoe].*)$/o$1/
    - xform/^([ae])(.*)$/$1$1$2/
```

这段配置实现了以下功能：

1. 通过`xlit`去除所有拼音声调
2. 通过`xform`处理特殊的音节，如将`ń`替换为`en`，将独立的`ng`替换为`eng`
3. 通过`derive`允许`u`和`v`互换（如`ju`/`jv`）
4. 通过`derive`允许在`a`/`o`/`e`开头的编码前加一个`o`
5. 通过`xform`将`a`或`e`开头的编码的首字母重复一次

这些规则共同作用，使得输入法能够处理各种拼音输入习惯，提供更灵活的输入体验。

## 源码解析

Rime转换规则在librime源码中的实现主要在以下文件中：

- `src/rime/algo/calculus.h`
- `src/rime/algo/calculus.cc`

这些规则都继承自基类`Calculation`：

```cpp
class Calculation {
 public:
  using Factory = Calculation*(const vector<string>& args);

  virtual ~Calculation() = default;
  virtual bool Apply(Spelling* spelling) = 0;
  virtual bool addition() { return true; }  // 是否添加新的输入
  virtual bool deletion() { return true; }  // 是否删除原始输入
};
```

各规则类的继承关系如下：

```
Calculation
├── Transliteration (xlit)
├── Transformation (xform)
│   └── Derivation (derive)
│       ├── Fuzzing (fuzz)
│       └── Abbreviation (abbrev)
└── Erasion (erase)
```

通过理解这些规则的工作原理和实现方式，我们可以更好地自定义Rime输入法的行为，满足个性化的输入需求。

---

本文档由[用户名]创建于2025年3月13日，基于librime源码和万象拼音配置文件分析。
