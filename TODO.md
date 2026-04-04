
---

## TODO/待确认点

### 1) data_types.hpp
**TODO/待确认点**
`kLandmarkDims = 2` 目前假设每个关键点只有 (x, y)。

**你需要做什么**
确认 MediaPipe 手关键点模型输出到底是：
- 21×2（x,y）还是
- 21×3（x,y,z）或 21×(x,y,score/visibility)

如果是 3 维：把 `kLandmarkDims` 改成 3，并同步检查 `landmark_detector.hpp/.cpp` 的读取逻辑。

### 2) landmark_detector.hpp
**TODO/待确认点**
`kLandmarkOutputElements = kNumLandmarks * kLandmarkDims`（目前=42）必须和 landmark 模型输出 float 数完全一致。

**你需要做什么**
向模型转换/算法同事要到 landmark 模型输出 tensor 的精确 shape / dtype（例如 [1, 42] float32 或 [1, 63] float32）。
如果不等于 42：调整 `kLandmarkDims` 或新增常量使它匹配真实输出，否则会造成：
- ssne_getoutput() 写入越界/不足（取决于底层实现）
- 或你读取的点位数据不正确。

### 3) landmark_detector.cpp
**TODO/待确认点**
代码里有两类“隐含 TODO”：

1. **归一化参数是否内嵌在模型中**
   - 现在调用了：`SetNormalize(pipe_offline, model_id)`
   - 如果返回非 0，会打印 warn。

   **你需要做什么：**
   - 确认 .m1model/.a1model 是否带 mean/std（有些模型转换流程会写进去，有些不会）。
   - 如果没有，需要你们明确用哪套 mean/std，并改成显式 SetNormalize2/SetNormalize3（取决于模型输入通道）。

2. **输出坐标是否真的 normalized 到 [0,1]**
   - 目前假设输出是 normalized，然后乘以 crop_width/crop_height 映射到 crop 像素。

   **你需要做什么：**
   - 确认模型输出坐标范围：
     - 若输出已经是像素坐标（例如直接是 0~255 或 0~crop_w），则这里的乘法需要改掉，否则坐标会爆掉。
   - 最简单的验证：打印几个点，看看是否都在 0~1 之间。

### 4) sign_recognizer.hpp
**TODO/待确认点（最关键）**
`static constexpr int kVocabSize = 100;` 这是占位值。

**你需要做什么**
找到 TCN/CTC 模型真实输出类别数（包含 blank）：
- 通常是：`vocab_size = gloss词表大小 + 1(blank)`
- 把 `kVocabSize` 改成真实值，否则：
  - `create_tensor(kVocabSize, seq_len, ...)` 分配的 output tensor 尺寸会不对
  - `ssne_getoutput()` 可能失败或写入不完整
  - CTC decode 也没法做对

### 5) sign_recognizer.cpp
**TODO/待确认点**
`CTC_Decode()` 还是 stub：固定返回 "HELLO"
注释里也写了：需要真实 vocab（id->gloss 的映射表）

**你需要做什么**
提供/确定：
1. blank_id 是不是 0（当前注释按 0 写的）
2. 词表文件/数组（例如 `std::vector<std::string> vocab`）
3. 模型输出 layout：是 [T, V] 还是 [V, T]，以及是否 log-softmax

然后把 `CTC_Decode()` 实现成：
- greedy：每帧 argmax -> collapse duplicates -> remove blank
- 或 beam search（如果你们需要更稳）

### 6) main.cpp
当前没有 TODO，但有“参数仍写死”
你之前强调“参数不固定、要预留参数”。目前 `img_w/img_h/crop_w/crop_h/crop_offset_y` 仍是写死在 main.cpp。

**你需要做什么（建议）**
决定参数来源（任选其一）：
- 命令行参数（最简单）
- JSON/YAML 配置文件
- 环境变量

然后把这些参数传下去即可（现在链路已经打通：camera/init + detector/init + visualizer/draw 都吃到了 crop_offset_y/crop_w/crop_h）。

---
