# librime-en2zh — Rust 重构

这是 librime（RIME 中州韻输入法引擎）核心引擎的 Rust 重构，位于 `rust/` 目录下，
是一个独立的 Cargo workspace，与原有 C++ 代码（`src/`）并存、互不影响。

## 构建与测试

```sh
cd rust
cargo build          # 构建 rime（rlib）+ rime-capi（cdylib/staticlib）
cargo test           # 运行全部单元/集成测试（当前 70 个）
```

零外部依赖：沙箱无网络，无法拉取 crates.io，故 `serde_yaml`/`rust-opencc`/`regex`
等第三方 crate 暂以自包含实现替代（见下文“偏离与说明”）。

## 结构

```
rust/
  Cargo.toml             workspace
  rime/                 核心库（rlib）
    src/
      key_table.rs       键值/键名/修饰键表（由 tools/gen_key_table.py 从 C 源码生成）
      key_event.rs       KeyEvent / KeySequence
      candidate.rs       Candidate trait + Simple/Shadow/Uniquified
      translation.rs     Translation trait + Unique/Fifo/Union/Merged/Cache/Distinct/Prefetch
      segmentation.rs    Segment / Segmentation
      composition.rs     Composition / Preedit
      menu.rs            Menu / Page
      commit_history.rs
      context.rs         Context（含信号通知机制）
      config.rs          ConfigItem / Config 配置树
      schema.rs          Schema
      ticket.rs          Ticket
      component.rs       组件工厂注册表（processor/segmentor/translator/filter/formatter）
      switches.rs        开关选项解析
      engine.rs          ConcreteEngine（ProcessKey→Compose→分段→翻译）
      service.rs         Session / Service（会话管理 + Rust API）
      algo/              拼写代数
        strings.rs spelling.rs calculus.rs algebra.rs syllabifier.rs
        encoder.rs utilities.rs regex.rs（自包含正则引擎）
      dict/              词典
        vocabulary.rs table.rs prism.rs dictionary.rs
      gear/              处理管道组件
        speller.rs abc_segmentor.rs echo_translator.rs
        table_translator.rs script_translator.rs
  rime-capi/            C ABI（cdylib/staticlib），兼容 rime_api.h 核心
    src/lib.rs
  tools/gen_key_table.py  键表生成脚本（一次性迁移工具）
```

## 已实现

- **基础设施**：键模型、候选、翻译器、分段/组合/菜单/上下文、提交历史
- **引擎层**：配置、schema、组件工厂、ConcreteEngine 完整流程（按键→组合→分段→翻译→候选/提交）
- **拼写代数**：calculus（xlit/xform/erase/derive/fuzz/abbrev/reorder）、algebra、自包含正则引擎、编码器
- **词典**：vocabulary / table（内存 trie 精确+前缀查询）/ prism（拼写→音节 trie）/ dictionary
- **配置与词典加载**：自包含 YAML 子集解析器（嵌套 map/列表/内联列表/引号/注释/块标量），
  `.dict.yaml` TSV 词典加载，可加载真实 `data/minimal/` 下的 schema 与词典
- **处理管道**：speller（按键入栈）、abc_segmentor（分段）、echo/table/script_translator（翻译）
- **会话 + API**：service（Rust API）+ rime-capi（C ABI：会话/按键/提交/上下文/状态/选项/config 读写）

最小可用引擎已验证：经 `RimeProcessKey` 逐键输入 `ninan`，引擎自动切分为
`ni`+`nan`，产出候选「呢喃」，可提交上屏。

## 待完成

1. **deploy/schema_list 等 rime_api.h 剩余函数** — 部署/方案列表
2. **OpenCC 简繁转换** — simplified/traditional
3. **更多 gear 组件** — punctuator、navigator、recognizer、key_binder 等

## 偏离与说明

目标原计划采用 `serde_yaml`、`rust-opencc` 等第三方 crate；因沙箱无网络访问，
本次以纯 `std` 自包含实现替代（自写正则引擎、内存词典、内存配置树），
接口已按未来可替换为第三方 crate 的方式设计。
