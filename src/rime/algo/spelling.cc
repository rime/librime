#include "spelling.h"

namespace rime {

void SpellingProperties::Compose(const SpellingProperties& delta) {
  // 類型取最模糊者
  if (delta.type > type) {
    type = delta.type;
  }
  // 權重累加
  credibility += delta.credibility;
  // 糾錯標記
  if (delta.is_correction) {
    is_correction = true;
  }

  if (!delta.tips.empty()) {
    tips = delta.tips;
  }
}

void SpellingProperties::Update(const SpellingProperties& other) {
  // 類型相同, 以權重高者爲主, 同步糾錯標記
  if (type == other.type) {
    is_correction = is_correction && other.is_correction;
  }
  // 採納更優的類型
  else if (other.type < type) {
    type = other.type;
    // 糾錯狀態跟隨類型變更
    is_correction = other.is_correction;
  }
  // 保留最大權重
  if (other.credibility > credibility) {
    credibility = other.credibility;
  }
  // 提示是針對其中一個拼寫來源的, 可能對合併後的拼寫不適用
  tips.clear();
}

}  // namespace rime
