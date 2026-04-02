#pragma once
#include "smartsoc/ssne_api.h"
#include "data_types.hpp"
#include <deque>
#include <string>

class SignRecognizer {
public:
    void Initialize(const std::string& tcn_model_path, int seq_length, int feature_dim);
    // 接收一帧关键点，返回识别的Gloss（如果没有结果则返回空字符串）
    std::string ProcessFrame(const LandmarkResult& current_frame);
    void Release();

private:
    uint16_t model_id = 0;
    int max_seq_length;
    int feature_dim;
    std::deque<std::vector<Point2D>> frame_buffer; // 滑动窗口
    
    ssne_tensor_t inputs[1];
    ssne_tensor_t outputs[1];

    std::string CTC_Decode(float* probs, int length); // CTC 解码辅助函数
};