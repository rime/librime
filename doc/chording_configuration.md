# 並擊方案配置與遷移文檔

`StreamingChordProcessor` 是專爲連續並擊（串流並擊）設計的按鍵處理器，旨在無縫支援跨平臺環境（具備 KeyUp 的桌面 GUI 與無 KeyUp 的 Emacs/終端），並原生相容聲韻調並擊、多擊單字及雙功能標點。


## 1. 核心機制說明

### A. 雙功能鍵（Dual-Role Keys）
* **並擊時**：充當方案自訂的和弦編碼（如調號、韻母）；
* **單擊時**：
  * **有 KeyUp 環境（GUI）**：手指釋放（Release）時立即回放原生按鍵交回下游（如 `punctuator` 上屏標點、`selector` 選詞）；
  * **無 KeyUp 環境（Emacs）**：超過容差窗口或按下後續按鍵時流式超時回放；
  * **連擊（Double Tap）**：雙擊同一個雙功能鍵時，直接結算首擊並吞掉第 2 擊重複碼。
* **待決視覺引導**：處於判定窗口期時，預編輯區即時展示 `[鍵符]`（空格顯示爲 `␣`），杜絕無 KeyUp 環境下的盲打疑慮。

### B. 智能和弦閉包與跨平臺隔音符（Delimiter Lifecycle）
* **桌面端（鼠鬚管/小狼毫，有 KeyUp）**：
  物理按鍵全釋放時，和弦自動規範化封閉（如 `[ZFURO]`）並標記關閉狀態。後續落鍵**絕不向緩衝區注入任何多餘的分隔符號**。多擊單字 `[A][B]` 緊密銜接，退格（BackSpace）時乾淨利落，無任何多餘空白或引號殘留。
* **終端與編輯器（emacs-rime，無 KeyUp）**：
  在連續輸入中遇到停頓（`Δ t > chord_timeout_ms`）或手系逆轉（韻母後接聲母）時，狀態機自動向緩衝區推入專屬隔音符號（默認取可見的 `'`），輔助下游語言模型與 Translator 精確進行長句分詞切分。


## 2. 自動推導規則（零配置升級）

只要按鍵符合以下兩項條件，引擎會**自動註冊**其爲雙功能鍵，**無需手動聲明**：
1. 該物理鍵在 `key_map` 中存在映射（參與並擊）；
2. 該鍵屬於非 ASCII 字母的字符（如空格、分號 `;`、單引號 `'`、逗號 `,`、斜線 `/` 等）。


## 3. 聲韻調並擊方案配置範例

編輯方案文件（如 `combo_pinyin_tone.schema.yaml`）：

```yaml
schema:
  schema_id: combo_pinyin_tone
  name: 宮保拼音·聲韻調並擊

engine:
  processors:
    - ascii_composer
    - recognizer
    - streaming_chord_processor  # 替代舊版 chord_composer
    - key_binder
    - punctuator
    - selector
    - navigator
    - express_editor
  # 其他組件定義，本文省略

speller:
  alphabet: 'SCZHLFGDBKTPIUVANREOXY[]'  # 包含音節定界符——方括號
  delimiter: " '"                       # 首位爲顯示空格，次位爲輸入切分單引號
  algebra:
    # 拼音轉並擊，本文省略
    __include: /pinyin_to_chord
    __append:
      # 聲調轉寫（一聲不標；二聲 X；三聲 XY；四聲 Y）
      - xform/[15]$//
      - xform/2$/X/
      - xform/3$/XY/
      - xform/4$/Y/
      # 單音節定界閉包
      # 和弦輸入由 translator/canonicalize 規格化後統一以閉包形式在棱鏡中檢索，避免抬鍵封閉的長和弦拆分爲兩段短和弦。
      # 以不帶調拼音爲例，SHUARO (爽) 可切分爲 SH UARO (失望)；和弦封閉後 [SHUARO] 與 [SH] [UARO] 無切分歧義。
      - xform/^.+$/[$0]/

translator:
  dictionary: terra_pinyin
  prism: combo_pinyin_tone
  enable_completion: false  # 只匹配完整的和弦閉包
  # 若爲無抬鍵環境，須規格化查詢片段，匹配棱鏡索引碼格式
  canonicalize:
    # 按照標準鍵序重排；將聲調鍵 X, Y 重排收納於音節閉包末尾
    - reorder SCZHLFGDBKTPIUVANREOXY
    # 和弦閉包用方括號定界
    - xform/^[A-Z]+$/[$0]/
  preedit_format:
    # 將並擊和弦代碼轉寫爲帶調拼音回顯
    __include: combo_pinyin_rules:/preedit_pinyin

streaming_chord:
  # 將佈局中的並擊鍵按指法分爲左右半區
  initial_keys: XSCZHLFGDBKTP
  final_keys: YIUVANREO
  chord_duration_ms: 60          # 同和弦微時差生理容差窗口 (ms)
  chord_timeout_ms: 120          # 異音節切分與組裝超時窗口 (ms)

  # [可選] 專屬串流隔音符號：
  # 若不配置，引擎會自動從 speller/delimiter 索取第一個非空格字符（即可見的 "'"）；
  # 若顯式設爲空字串 ""，則在無 KeyUp 環境下亦不向 input 注入任何字符。
  # delimiter: "'"

  key_map:
    # 宮保拼音七指禪佈局標準鍵位
    __include: combo_pinyin_rules:/key_map
    # 標點/空格參與並擊，引擎自動推導爲雙功能鍵：
    semicolon: 'Y'               # 並擊時爲調號 Y；單擊時回放輸出實體標點「；」
    space: 'A'                   # 並擊時爲韻母 A；單擊時回放用於選詞或輸出原生空格

  # [可選] 雙功能鍵白名單（顯式優先原則）：
  # 一旦顯式宣告，引擎將完全關閉非字母鍵自動推導，僅對名單內的按鍵生效
  # dual_role_keys:
  #   - semicolon
  #   - space

  # 抬鍵盡釋後，對該音節執行定界閉包規範化
  canonicalize:
    # 按照標準鍵序重排；將聲調鍵 X, Y 重排收納於音節閉包末尾
    - reorder SCZHLFGDBKTPIUVANREOXY
    # 和弦閉包用方括號定界
    - xform/^[A-Z]+$/[$0]/

editor:
  bindings:
    BackSpace: back_syllable  # 刪除整個音節
    Return: confirm

punctuator:
  # 標點符號定義，本文僅展示局部
  half_shape:
    ';': '；'

recognizer:
  patterns:
    # 不得引入大寫模式定義，防止截胡大寫並擊碼
    # uppercase: ''
