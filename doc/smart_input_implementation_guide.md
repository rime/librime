# RIME输入法智能输入功能实现指南

本文档详细介绍了如何在RIME输入法框架中实现类似Gboard的智能输入功能，包括可行性分析和详细的技术方案。

## 目录

1. [功能理解](#功能理解)
2. [可行性分析](#可行性分析)
3. [技术方案](#技术方案)
4. [行动方案](#行动方案)
5. [优先实施功能](#优先实施功能)
   - [基础模糊输入容错](#1-基础模糊输入容错)
   - [简单的n-gram语言模型](#2-简单的n-gram语言模型)
   - [上下文感知的候选词排序](#3-上下文感知的候选词排序)
   - [用户个性化学习](#4-用户个性化学习)
6. [集成方案](#集成方案)

## 功能理解

Gboard所实现的核心功能包括：
1. 神经空间模型（NSM）- 将模糊触摸序列映射到键盘按键
2. 有限状态转换器（FST）- 用于词汇和语法约束的语言建模
3. 多语言支持和音译功能
4. 预测/建议系统和自动纠错机制

## 可行性分析

### 优势：
1. **现有架构基础**: librime已经有输入法框架的核心组件，如引擎、上下文处理等
2. **模块化设计**: RIME的设计允许通过添加新组件来扩展功能
3. **开源生态**: 可以利用现有的开源机器学习库如TensorFlow Lite

### 挑战：
1. **资源限制**: Gboard由Google开发，拥有大量数据和计算资源，RIME可能受限
2. **实现复杂性**: 神经空间模型和FST解码器的实现较为复杂
3. **训练数据获取**: 缺乏类似Google的大规模用户反馈数据用于训练模型

## 技术方案

### 1. 空间模型实现
- **简化方案**: 可以实现一个基于高斯模型的简化版本，作为起点
- **进阶方案**: 集成轻量级LSTM模型（如使用TensorFlow Lite）处理触摸模糊性

### 2. 语言模型实现
- 使用开源的FST库（如OpenFst）构建语言模型转换器
- 构建n-gram语言模型预测下一个可能的词语
- 为中文输入设计特定的上下文预测算法

### 3. 自动纠错机制
- 实现编辑距离算法检测和纠正错误
- 建立常见错误的纠正规则库
- 使用统计方法计算用户输入的可能正确形式

## 行动方案

### 第一阶段：基础设施搭建（1-2个月）
1. **调研与学习**:
   - 深入研究librime现有架构
   - 学习FST和神经网络在输入法中的应用
   - 研究TensorFlow Lite等小型设备上的机器学习实现

2. **扩展架构**:
   - 设计并实现模糊输入处理框架
   - 添加语言模型组件接口
   - 修改上下文处理，支持基于统计的预测功能

### 第二阶段：核心算法实现（2-3个月）
1. **空间模型实现**:
   - 实现基础的触摸模型（可以从简单的高斯模型开始）
   - 为触屏键盘设计坐标映射系统
   - 添加手势输入的轨迹分析

2. **语言模型实现**:
   - 集成OpenFst库
   - 实现n-gram模型处理
   - 构建中文词汇和语法模型

3. **预测与纠错**:
   - 实现候选词排序算法
   - 添加错误检测与纠正功能
   - 设计用户反馈学习机制

### 第三阶段：训练与优化（1-2个月）
1. **数据收集**:
   - 设计匿名数据收集机制（需征得用户同意）
   - 构建训练数据集
   - 准备评估指标和测试集

2. **模型训练**:
   - 训练初始的空间和语言模型
   - 针对中文输入特点优化模型
   - 调整模型大小和性能以适应不同设备

3. **性能优化**:
   - 优化内存使用和速度
   - 减小模型大小
   - 实现增量学习机制

### 第四阶段：集成与测试（1个月）
1. **集成到RIME**:
   - 与现有组件集成
   - 添加配置接口
   - 实现多语言支持

2. **测试与调优**:
   - 进行大规模测试
   - 收集用户反馈
   - 持续改进模型

## 优先实施功能

考虑到资源限制，我们优先实施以下功能：

### 1. 基础模糊输入容错

#### 技术架构

创建一个新的处理器类`FuzzyInputProcessor`，处理输入时的触摸偏移和拼写错误。

```cpp
// fuzzy_input_processor.h
class FuzzyInputProcessor : public Processor {
 public:
  FuzzyInputProcessor(const Ticket& ticket);
  virtual ~FuzzyInputProcessor();
  
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);
  
 private:
  // 键盘布局信息，用于计算物理位置
  struct KeyboardLayout {
    std::map<int, std::pair<double, double>> key_positions;
  };
  
  // 加载配置
  void LoadConfig(Schema* schema);
  
  // 计算两个按键之间的空间距离
  double CalculateDistance(int key1, int key2);
  
  // 获取给定按键的邻近按键
  std::vector<int> GetNeighboringKeys(int key);
  
  // 根据距离计算可能性
  double CalculateProbability(double distance);
  
  KeyboardLayout keyboard_layout_;
  double proximity_threshold_;  // 邻近键的距离阈值
  bool enable_fuzzy_input_;     // 是否启用模糊输入
};
```

#### 实现细节

1. **键盘布局定义**：
   ```yaml
   # keyboard_layouts.yaml
   qwerty:
     q: [0, 0]
     w: [1, 0]
     # 每个键的坐标，单位化为网格位置
   ```

2. **空间距离计算**：
   ```cpp
   double FuzzyInputProcessor::CalculateDistance(int key1, int key2) {
     if (keyboard_layout_.key_positions.find(key1) == keyboard_layout_.key_positions.end() ||
         keyboard_layout_.key_positions.find(key2) == keyboard_layout_.key_positions.end()) {
       return 999.0;  // 无效距离
     }
     
     auto pos1 = keyboard_layout_.key_positions[key1];
     auto pos2 = keyboard_layout_.key_positions[key2];
     
     return sqrt(pow(pos1.first - pos2.first, 2) + 
                 pow(pos1.second - pos2.second, 2));
   }
   ```

3. **处理模糊输入**：
   ```cpp
   ProcessResult FuzzyInputProcessor::ProcessKeyEvent(const KeyEvent& key_event) {
     if (!enable_fuzzy_input_)
       return kNoop;
       
     int keycode = key_event.keycode();
     if (!key_event.ctrl() && !key_event.alt() && !key_event.super() &&
         !key_event.release()) {
       // 获取邻近键
       auto neighboring_keys = GetNeighboringKeys(keycode);
       
       // 将原始按键及其邻近键加入候选
       auto context = engine_->context();
       Segment& segment = context->composition().back();
       
       // 添加原始输入
       segment.tags.insert("fuzzy");
       
       // 添加可能的邻近按键
       for (int key : neighboring_keys) {
         // 计算概率，加入候选
         double prob = CalculateProbability(CalculateDistance(keycode, key));
         segment.AddFuzzyCandidate(key, prob);
       }
       
       return kAccepted;
     }
     
     return kNoop;
   }
   ```

### 2. 简单的n-gram语言模型

#### 技术架构

创建一个n-gram语言模型类和处理器，处理词语预测。

```cpp
// n_gram_model.h
class NGramModel {
 public:
  NGramModel();
  ~NGramModel();
  
  // 加载语料库训练模型
  bool Load(const std::string& file_path);
  
  // 根据前文预测下一个词
  std::vector<std::pair<std::string, double>> Predict(
      const std::vector<std::string>& context, size_t max_predictions = 5);
  
  // 更新模型频率
  void Update(const std::vector<std::string>& sequence);
  
  // 保存模型
  bool Save(const std::string& file_path);
  
 private:
  // 使用字典树存储n-gram
  struct TrieNode {
    std::string word;
    int frequency;
    std::map<std::string, std::shared_ptr<TrieNode>> children;
  };
  
  std::shared_ptr<TrieNode> root_;
  size_t max_n_gram_length_;  // 最大n-gram长度，如3表示trigram
};

// n_gram_processor.h
class NGramProcessor : public Processor {
 public:
  NGramProcessor(const Ticket& ticket);
  virtual ~NGramProcessor();
  
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);
  
 private:
  void LoadConfig(Schema* schema);
  void UpdatePredictions();
  
  NGramModel model_;
  std::string model_file_;
  size_t max_predictions_;
  bool enable_prediction_;
};
```

#### 实现细节

1. **模型格式**：
   ```
   # ngram_model.txt - 简化格式
   我 100
   你 85
   我 喜欢 50
   你 好 45
   我 喜欢 编程 25
   ```

2. **加载模型**：
   ```cpp
   bool NGramModel::Load(const std::string& file_path) {
     std::ifstream file(file_path);
     if (!file.is_open()) {
       return false;
     }
     
     root_ = std::make_shared<TrieNode>();
     root_->frequency = 0;
     
     std::string line;
     while (std::getline(file, line)) {
       std::istringstream iss(line);
       std::vector<std::string> tokens;
       std::string token;
       
       while (iss >> token) {
         tokens.push_back(token);
       }
       
       if (tokens.size() < 2) continue;
       
       int frequency = std::stoi(tokens.back());
       tokens.pop_back();  // 移除频率值
       
       // 插入n-gram
       auto current = root_;
       for (const auto& word : tokens) {
         if (current->children.find(word) == current->children.end()) {
           current->children[word] = std::make_shared<TrieNode>();
           current->children[word]->word = word;
           current->children[word]->frequency = 0;
         }
         current = current->children[word];
       }
       current->frequency += frequency;
     }
     
     return true;
   }
   ```

3. **预测下一个词**：
   ```cpp
   std::vector<std::pair<std::string, double>> NGramModel::Predict(
       const std::vector<std::string>& context, size_t max_predictions) {
     std::vector<std::pair<std::string, double>> predictions;
     
     // 找到context在trie中的位置
     auto current = root_;
     for (const auto& word : context) {
       if (current->children.find(word) == current->children.end()) {
         // 未找到完整context，尝试回退
         break;
       }
       current = current->children[word];
     }
     
     // 从当前节点获取所有子节点作为预测
     std::vector<std::pair<std::string, int>> candidates;
     for (const auto& child : current->children) {
       candidates.push_back({child.first, child.second->frequency});
     }
     
     // 按频率排序
     std::sort(candidates.begin(), candidates.end(),
         [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
           return a.second > b.second;
         });
     
     // 转换为概率
     int total_freq = 0;
     for (const auto& candidate : candidates) {
       total_freq += candidate.second;
     }
     
     for (size_t i = 0; i < std::min(max_predictions, candidates.size()); ++i) {
       double prob = static_cast<double>(candidates[i].second) / total_freq;
       predictions.push_back({candidates[i].first, prob});
     }
     
     return predictions;
   }
   ```

### 3. 上下文感知的候选词排序

#### 技术架构

创建一个新的过滤器，对候选词进行排序。

```cpp
// context_filter.h
class ContextAwareFilter : public Filter {
 public:
  ContextAwareFilter(const Ticket& ticket);
  
  virtual void Apply(CandidateList* recruited,
                     CandidateList* candidates);
                     
 private:
  void LoadConfig(Schema* schema);
  
  // 计算候选词与上下文的相关性分数
  double CalculateContextScore(const an<Candidate>& candidate,
                              const Context* context);
  
  // 提取上下文信息
  std::vector<std::string> GetContextWords(const Context* context);
  
  NGramModel* language_model_;  // 使用上面定义的n-gram模型
  bool enable_context_awareness_;
  double context_weight_;  // 上下文权重
};
```

#### 实现细节

1. **配置格式**：
   ```yaml
   # context_filter.yaml
   context_aware_filter:
     enabled: true
     context_weight: 0.6
     history_size: 5  # 考虑上文的词语数量
   ```

2. **候选词排序实现**：
   ```cpp
   void ContextAwareFilter::Apply(CandidateList* recruited,
                                 CandidateList* candidates) {
     if (!enable_context_awareness_ || !candidates || candidates->empty())
       return;
     
     auto context = engine_->context();
     std::vector<std::string> context_words = GetContextWords(context);
     
     if (context_words.empty()) {
       return;  // 无上下文信息，使用默认排序
     }
     
     // 为每个候选计算上下文相关分数
     std::vector<std::pair<an<Candidate>, double>> scored_candidates;
     for (auto& candidate : *candidates) {
       double context_score = CalculateContextScore(candidate, context);
       double original_score = candidate->quality();
       // 综合分数 = 原始分数 * (1 - 权重) + 上下文分数 * 权重
       double final_score = original_score * (1 - context_weight_) + 
                            context_score * context_weight_;
       
       scored_candidates.push_back({candidate, final_score});
     }
     
     // 按综合分数排序
     std::sort(scored_candidates.begin(), scored_candidates.end(),
         [](const std::pair<an<Candidate>, double>& a,
            const std::pair<an<Candidate>, double>& b) {
           return a.second > b.second;
         });
     
     // 重构候选列表
     candidates->clear();
     for (auto& pair : scored_candidates) {
       candidates->push_back(pair.first);
     }
   }
   ```

3. **计算上下文相关性**：
   ```cpp
   double ContextAwareFilter::CalculateContextScore(
       const an<Candidate>& candidate,
       const Context* context) {
     std::string candidate_text = candidate->text();
     std::vector<std::string> context_words = GetContextWords(context);
     
     // 使用n-gram模型计算该候选词在当前上下文中的概率
     auto prediction = language_model_->Predict(context_words);
     
     for (const auto& pred : prediction) {
       if (pred.first == candidate_text) {
         return pred.second;  // 返回模型预测的概率
       }
     }
     
     return 0.0;  // 未在预测中找到
   }
   ```

### 4. 用户个性化学习

#### 技术架构

创建一个用户习惯学习系统，记录和应用用户选择。

```cpp
// user_dictionary.h
class UserDictionary : public Class<UserDictionary, Service> {
 public:
  UserDictionary(const Ticket& ticket);
  virtual ~UserDictionary();
  
  // 记录用户选择
  void RecordSelection(const std::string& context, 
                       const std::string& selected_word);
  
  // 根据用户历史获取推荐
  std::vector<std::pair<std::string, double>> GetUserRecommendations(
      const std::string& context, size_t limit = 5);
  
  // 定期保存
  bool SaveToFile();
  
 private:
  bool LoadFromFile();
  
  // 用户选择记录
  struct UserSelection {
    std::string context;
    std::string word;
    int count;
    time_t last_used;
  };
  
  std::string user_dict_file_;
  std::multimap<std::string, UserSelection> user_selections_;
  size_t max_history_size_;
  bool enable_user_dictionary_;
  double user_dict_weight_;  // 个性化权重
};

// user_dictionary_processor.h
class UserDictionaryProcessor : public Processor {
 public:
  UserDictionaryProcessor(const Ticket& ticket);
  virtual ~UserDictionaryProcessor();
  
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);
  
 private:
  void LoadConfig(Schema* schema);
  void UpdateUserDictionary(const Context* context);
  void OnCommit(Context* ctx);
  
  UserDictionary* user_dictionary_;
  Connection commit_connection_;
};
```

#### 实现细节

1. **用户词典格式**：
   ```yaml
   # user_dict.yaml
   entries:
     - context: "我想"
       word: "去"
       count: 15
       last_used: 1712499424  # Unix时间戳
     - context: "早上好"
       word: "啊"
       count: 8
       last_used: 1712485024
   ```

2. **记录用户选择**：
   ```cpp
   void UserDictionary::RecordSelection(const std::string& context,
                                       const std::string& selected_word) {
     if (!enable_user_dictionary_)
       return;
       
     // 检查是否已有该记录
     auto range = user_selections_.equal_range(context);
     for (auto it = range.first; it != range.second; ++it) {
       if (it->second.word == selected_word) {
         // 已有记录，更新计数和时间
         it->second.count++;
         it->second.last_used = time(NULL);
         return;
       }
     }
     
     // 创建新记录
     UserSelection selection;
     selection.context = context;
     selection.word = selected_word;
     selection.count = 1;
     selection.last_used = time(NULL);
     
     user_selections_.insert({context, selection});
     
     // 如果记录过多，清理旧记录
     if (user_selections_.size() > max_history_size_) {
       // 找到最旧/最少使用的记录删除
       auto oldest = user_selections_.begin();
       time_t oldest_time = oldest->second.last_used;
       int lowest_count = oldest->second.count;
       
       for (auto it = user_selections_.begin(); it != user_selections_.end(); ++it) {
         if (it->second.count < lowest_count ||
             (it->second.count == lowest_count && it->second.last_used < oldest_time)) {
           oldest = it;
           oldest_time = it->second.last_used;
           lowest_count = it->second.count;
         }
       }
       
       user_selections_.erase(oldest);
     }
   }
   ```

3. **获取用户推荐**：
   ```cpp
   std::vector<std::pair<std::string, double>> UserDictionary::GetUserRecommendations(
       const std::string& context, size_t limit) {
     std::vector<std::pair<std::string, double>> recommendations;
     
     if (!enable_user_dictionary_)
       return recommendations;
     
     // 查找匹配当前上下文的记录
     auto range = user_selections_.equal_range(context);
     
     // 收集所有匹配记录
     std::vector<UserSelection> matches;
     for (auto it = range.first; it != range.second; ++it) {
       matches.push_back(it->second);
     }
     
     // 按计数和最近使用时间排序
     std::sort(matches.begin(), matches.end(),
         [](const UserSelection& a, const UserSelection& b) {
           // 使用计数作为主要排序依据，最近使用时间为次要
           if (a.count != b.count)
             return a.count > b.count;
           return a.last_used > b.last_used;
         });
     
     // 计算总计数以获取概率
     int total_count = 0;
     for (const auto& match : matches) {
       total_count += match.count;
     }
     
     // 转换为概率分数
     for (size_t i = 0; i < std::min(limit, matches.size()); ++i) {
       double score = static_cast<double>(matches[i].count) / total_count;
       recommendations.push_back({matches[i].word, score});
     }
     
     return recommendations;
   }
   ```

4. **挂钩用户提交事件**：
   ```cpp
   UserDictionaryProcessor::UserDictionaryProcessor(const Ticket& ticket)
     : Processor(ticket) {
     LoadConfig(ticket.schema);
     
     // 创建用户词典
     user_dictionary_ = UserDictionary::Require(ticket.schema);
     
     // 监听提交事件
     commit_connection_ = engine_->context()->commit_notifier().connect(
         [this](Context* ctx) { this->OnCommit(ctx); });
   }
   
   void UserDictionaryProcessor::OnCommit(Context* ctx) {
     // 获取上下文和提交的词
     std::string context_text = "";
     if (ctx->composition().size() > 1) {
       size_t ctx_size = ctx->composition().size() - 1;
       for (size_t i = 0; i < ctx_size; ++i) {
         if (!context_text.empty())
           context_text += " ";
         context_text += ctx->composition()[i].GetSelectedText();
       }
     }
     
     std::string committed_text = ctx->composition().back().GetSelectedText();
     
     // 记录用户选择
     user_dictionary_->RecordSelection(context_text, committed_text);
   }
   ```

## 集成方案

这四个功能组件需要整合到RIME的现有架构中。以下是整合方案：

1. **扩展engine.cc**：
   - 注册新创建的处理器和过滤器
   - 确保处理顺序合理（模糊输入处理 -> n-gram处理 -> 用户词典处理）

2. **扩展schema.yaml**：
   ```yaml
   # 在schema.yaml中添加新组件配置
   engine:
     processors:
       # 现有处理器
       - ascii_composer
       - recognizer
       - key_binder
       - speller
       # 新增处理器
       - fuzzy_input
       - n_gram_processor
       - user_dictionary_processor
       # 其他处理器
       
     segmentors:
       # 现有分段器
       
     translators:
       # 现有转换器
       
     filters:
       # 现有过滤器
       # 新增过滤器
       - context_aware_filter
       # 其他过滤器
   
   # 新组件配置
   fuzzy_input:
     enable: true
     proximity_threshold: 1.5
     
   n_gram_model:
     enable: true
     model_file: n_gram_data.txt
     max_predictions: 5
     
   context_aware_filter:
     enable: true
     context_weight: 0.6
     history_size: 5
     
   user_dictionary:
     enable: true
     user_dict_file: user_dict.yaml
     max_history_size: 1000
     user_dict_weight: 0.7
   ```

3. **性能优化考虑**：
   - 使用缓存减少重复计算
   - 对大型模型采用内存映射文件
   - 延迟加载策略减少启动时间
   - 异步保存用户数据防止阻塞UI

通过这套技术方案，您可以在RIME框架中实现类似Gboard的智能输入功能，同时保持RIME的模块化设计理念和扩展性。
