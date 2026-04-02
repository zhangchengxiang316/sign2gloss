#include "sign_recognizer.hpp"
#include <iostream>

void SignRecognizer::Initialize(const std::string& tcn_model_path, int seq_length, int feature_dim) {
    max_seq_length = seq_length;
    this->feature_dim = feature_dim;
    model_id = ssne_loadmodel(const_cast<char*>(tcn_model_path.c_str()), SSNE_STATIC_ALLOC);
    
    // TCN 输入通常是一个 [seq_length, feature_dim] 的一维或二维矩阵，在这里映射为一维 tensor
    inputs[0] = create_tensor(feature_dim, seq_length, SSNE_FLOAT32, SSNE_BUF_AI);
}

std::string SignRecognizer::ProcessFrame(const LandmarkResult& current_frame) {
    if (!current_frame.is_detected) return ""; // 无效帧跳过

    // 1. 加入缓冲
    frame_buffer.push_back(current_frame.landmarks);
    if (frame_buffer.size() < max_seq_length) {
        return ""; // 帧数不够，继续收集
    }
    if (frame_buffer.size() > max_seq_length) {
        frame_buffer.pop_front(); // 保持滑动窗口大小
    }

    // 2. 将 frame_buffer 整理到 inputs[0] 中
    float* input_ptr = (float*)get_data(inputs[0]);
    for (int t = 0; t < max_seq_length; ++t) {
        for (int p = 0; p < current_frame.landmarks.size(); ++p) {
            int base_idx = t * feature_dim + p * 2;
            input_ptr[base_idx] = frame_buffer[t][p].x;
            input_ptr[base_idx + 1] = frame_buffer[t][p].y;
        }
    }

    // 3. 执行 TCN 推理
    ssne_inference(model_id, 1, inputs);
    ssne_getoutput(model_id, 1, outputs);

    // 4. 解析输出并 CTC 解码
    float* out_probs = (float*)get_data(outputs[0]);
    // TODO: 根据你的模型输出 shape 修改解码逻辑
    std::string gloss = CTC_Decode(out_probs, max_seq_length); 
    
    return gloss;
}

std::string SignRecognizer::CTC_Decode(float* probs, int length) {
    // 这里实现你的 CTC 贪心解码或者 BeamSearch
    // 伪代码：
    // string result = "";
    // int last_class = 0;
    // for t in time_steps:
    //    int max_class = argmax(probs[t]);
    //    if max_class != blank_id and max_class != last_class:
    //        result += vocab[max_class];
    //    last_class = max_class;
    // return result;
    return "HELLO"; // 测试桩
}

void SignRecognizer::Release() {
    release_tensor(inputs[0]);
    release_tensor(outputs[0]);
}