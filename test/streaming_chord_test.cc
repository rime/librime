#include <gtest/gtest.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/setup.h>
#include <chrono>
#include <thread>

using namespace rime;

class StreamingChordDualRoleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SetupLogging("rime.test");
    engine_.reset(Engine::Create());

    auto* schema = engine_->schema();
    auto* config = schema->config();
    config->SetString("schema/schema_id", "test_chord");

    // 聲母集與韻母集
    config->SetString("streaming_chord/initial_keys", "S");
    config->SetString("streaming_chord/final_keys", "YAU");
    config->SetInt("streaming_chord/chord_duration_ms", 60);
    config->SetInt("streaming_chord/chord_timeout_ms", 120);
    config->SetString("streaming_chord/delimiter", "'");

    // 鍵位映射：
    // s -> S (聲母), semicolon -> Y (調號), space -> A (雙功能韻母), u -> U
    // (韻母)
    config->SetString("streaming_chord/key_map/s", "S");
    config->SetString("streaming_chord/key_map/semicolon", "Y");
    config->SetString("streaming_chord/key_map/space", "A");
    config->SetString("streaming_chord/key_map/u", "U");

    // 只掛載被測組件本身
    auto processors = New<ConfigList>();
    processors->Append(New<ConfigValue>("streaming_chord_processor"));
    config->SetItem("engine/processors", processors);

    // 引擎原有方案修改配置後原位重新加載
    engine_->ApplySchema(schema);
  }

  void TearDown() override { engine_.reset(); }

  the<Engine> engine_;
};

// 1. 測試 GUI 環境有 KeyUp：單擊分號抬手立即上屏原生分號
TEST_F(StreamingChordDualRoleTest, SoloTapWithReleaseSemicolon) {
  Context* ctx = engine_->context();

  // 按下分號：暫存，展示待判定提示符 [;]
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_semicolon, 0)));
  EXPECT_TRUE(ctx->input().empty());
  ASSERT_FALSE(ctx->composition().empty());
  EXPECT_EQ(ctx->composition().back().prompt, "[;]");

  // 抬起分號：確證爲單擊，回放原生按鍵，下游無人截獲時兜底上屏原生 ";"
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_semicolon, kReleaseMask)));
  EXPECT_TRUE(ctx->input().empty());
  EXPECT_TRUE(ctx->composition().empty());
  ASSERT_FALSE(ctx->commit_history().empty());
  EXPECT_EQ(ctx->commit_history().back().text, ";");
}

// 2. 測試無 KeyUp 環境 (Emacs)：超時後敲擊下一音節，分號結算上屏，新鍵開闢新詞
TEST_F(StreamingChordDualRoleTest, SoloTapWithoutReleaseStreamingReplay) {
  Context* ctx = engine_->context();

  // 按下分號：暫存，展示待判定提示符 [;]
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_semicolon, 0)));
  EXPECT_EQ(ctx->composition().back().prompt, "[;]");

  // 超過 60ms 生理窗口 (等待 70ms)
  std::this_thread::sleep_for(std::chrono::milliseconds(70));

  // 敲下下一個漢字的聲母 s
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_s, 0)));

  // 暫存的分號被成功回放上屏
  ASSERT_FALSE(ctx->commit_history().empty());
  EXPECT_EQ(ctx->commit_history().back().text, ";");

  // 後續鍵 s 被順暢接納爲新音節，未被遺棄
  EXPECT_EQ(ctx->input(), "S");
}

// 3. 測試正向並擊 (先聲母 s 後分號 ;)
TEST_F(StreamingChordDualRoleTest, ForwardChordTone) {
  Context* ctx = engine_->context();

  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_s, 0)));
  EXPECT_EQ(ctx->input(), "S");

  // 20ms 內按下分號 ; 直接視爲並擊調號 Y 推入
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_semicolon, 0)));
  EXPECT_EQ(ctx->input(), "SY");
}

// 4. 測試逆向微時差並擊 (先分號 ; 後聲母 s)
TEST_F(StreamingChordDualRoleTest, ReverseChordToneWithPendingRecovery) {
  Context* ctx = engine_->context();

  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_semicolon, 0)));
  EXPECT_EQ(ctx->composition().back().prompt, "[;]");
  EXPECT_TRUE(ctx->input().empty());

  // 30ms 內 (<= 60ms) s 觸底：解凍暫存的分號爲 Y，隨後推入 S
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_s, 0)));
  EXPECT_EQ(ctx->input(), "YS");
}

// 5. 測試未映射按鍵直接穿透
TEST_F(StreamingChordDualRoleTest, UnmappedKeyBypassesDualRole) {
  Context* ctx = engine_->context();

  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_Tab, 0)));
  EXPECT_TRUE(ctx->composition().empty());
}

// 6. 測試場景：非連擊雙空格（空格 1 超時重發上屏，空格 2 留存並與 u 熔合爲 AU）
TEST_F(StreamingChordDualRoleTest,
       NonDoubleTapSpaceReplaysFirstAndFusesSecondWithChordKey) {
  Context* ctx = engine_->context();

  // 按下空格 1：暫存待決，顯示提示符 ␣
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_space, 0)));
  ASSERT_FALSE(ctx->composition().empty());
  EXPECT_EQ(ctx->composition().back().prompt, "␣");

  // 間隔 150ms (> 120ms chord_timeout_ms_，穩固超過超時窗口，非連擊)
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  // 按下空格 2：確證非快速雙擊，空格 1 結算上屏原生空格，空格 2 接替暫存
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_space, 0)));
  ASSERT_FALSE(ctx->commit_history().empty());
  EXPECT_EQ(ctx->commit_history().back().text, " ");
  // 空格 2 必須維持暫存狀態，絕不能被當作雙擊直接吞噬
  ASSERT_FALSE(ctx->composition().empty());
  EXPECT_EQ(ctx->composition().back().prompt, "␣");
  EXPECT_TRUE(ctx->input().empty());

  // 連續敲下字母 u (Δt < 1ms <= 60ms 並擊窗口，比照測試 3 與 4 避免 CI
  // 調度超時)
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_u, 0)));

  // 空格 2 成功解凍爲韻母 A，並與 U 組合爲新和弦輸入碼 AU
  EXPECT_EQ(ctx->input(), "AU");
  // 提示標籤清空
  EXPECT_TRUE(ctx->composition().empty() ||
              !ctx->composition().back().HasTag("chord_prompt"));
}

// 7.
// 測試隔音符場景：與前一和弦間隔超時，解凍暫存鍵（A）開闢新和弦時自動注入隔音符
// '
TEST_F(StreamingChordDualRoleTest,
       TimeoutBeforeChordStartingWithDualRoleKeyInsertsDelimiter) {
  Context* ctx = engine_->context();

  // 敲下前一個和弦的聲母 S
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_s, 0)));
  EXPECT_EQ(ctx->input(), "S");

  // 思考停頓 150ms (> 120ms chord_timeout_ms_，穩固超過超時窗口)
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  // 敲下空格（暫存爲待決韻母 A）
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_space, 0)));
  EXPECT_EQ(ctx->composition().back().prompt, "␣");
  EXPECT_EQ(ctx->input(), "S");

  // 連續敲下字母 u 觸發空格解凍 (Δt < 1ms <= 60ms 並擊窗口)
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_u, 0)));

  // 解凍過程經過 HandleChordKey 檢測到時序超時，必須在 S 與 AU 之間插入隔音符 '
  EXPECT_EQ(ctx->input(), "S'AU");
}
