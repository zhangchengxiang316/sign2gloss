#pragma once
#include "smartsoc/ssne_api.h"
#include "data_types.hpp"
#include <string>

class LandmarkDetector {
public:
    void Initialize(const std::string& model_path, int input_w, int input_h);
    void Predict(ssne_tensor_t* img_in, LandmarkResult* result);
    void Release();

private:
    uint16_t model_id = 0;
    ssne_tensor_t inputs[1];
    ssne_tensor_t outputs[1]; // 假设模型输出一个包含坐标的 tensor
    AiPreprocessPipe pipe_offline;
    int input_width, input_height;
};